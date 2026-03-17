import json
from typing import Optional, List
import torch
from torch.utils.data import Dataset
from transformers import AutoTokenizer
from config import Config
import nltk
import os


class CircumstanceDataset(Dataset):
    def __init__(self, texts, labels=None, tokenizer=None, max_len=Config.MAX_LEN):
        """
        Инициализация датасета.
        
        Args:
            texts: Список текстов
            labels: Список последовательностей меток (опционально)
            tokenizer: Токенизатор (если None, загружается из Config)
            max_len: Максимальная длина последовательности
        """
        self.texts = texts
        self.labels = labels
        # Используем AutoTokenizer, который загружает быстрый токенизатор если доступен
        self.tokenizer = tokenizer or AutoTokenizer.from_pretrained(Config.MODEL_NAME)
        self.max_len = max_len

        # Проверяем, что токенизатор быстрый
        if not self.tokenizer.is_fast:
            print("Warning: Using slow tokenizer. Word ids may not be available.")

    def __len__(self):
        """Возвращает количество примеров в датасете."""
        return len(self.texts)

    def __getitem__(self, idx):
        """
        Получение элемента датасета по индексу.
        
        Args:
            idx: Индекс элемента
            
        Returns:
            Словарь с тензорами input_ids, attention_mask и опционально labels
        """
        text = self.texts[idx]

        # Токенизация с использованием encode_plus для совместимости
        encoding = self.tokenizer.encode_plus(
            text,
            add_special_tokens=True,
            max_length=self.max_len,
            padding="max_length",
            truncation=True,
            return_attention_mask=True,
            return_tensors="pt",
        )

        item = {
            "input_ids": encoding["input_ids"].flatten(),
            "attention_mask": encoding["attention_mask"].flatten(),
        }

        if self.labels is not None:
            labels = self.labels[idx]
            # Выравниваем метки с токенами
            label_ids = self.align_labels_with_tokens(text, labels, encoding)

            # Паддинг меток
            padded_labels = torch.zeros(self.max_len, dtype=torch.long)
            padded_labels[: len(label_ids)] = torch.tensor(label_ids)

            item["labels"] = padded_labels

        return item

    def align_labels_with_tokens(self, text, original_labels, tokenized):
        """
        Выравнивание меток с субтокенами BERT.
        
        Args:
            text: Исходный текст
            original_labels: Исходные метки для слов
            tokenized: Результат токенизации
            
        Returns:
            Список ID меток для каждого токена
        """
        # Получаем word_ids
        if hasattr(tokenized, "word_ids") and callable(tokenized.word_ids):
            word_ids = tokenized.word_ids()
        else:
            word_ids = self._get_word_ids_fallback(text, tokenized["input_ids"])

        # Получаем список токенов
        tokens = self.tokenizer.convert_ids_to_tokens(tokenized["input_ids"][0])

        aligned_labels = []
        previous_word_id = None
        previous_label = None
        previous_bio_prefix = None

        for i, word_id in enumerate(word_ids): # type: ignore
            token = tokens[i]
            
            # Обработка специальных случаев
            if self._should_ignore_token(token, word_id):
                aligned_labels.append(-100)
                previous_word_id, previous_label, previous_bio_prefix = None, None, None
                continue
                
            # Обработка нового слова
            if word_id != previous_word_id:
                label_id, previous_label, previous_bio_prefix = self._process_new_word(
                    word_id, original_labels
                )
                aligned_labels.append(label_id)
                previous_word_id = word_id
            # Обработка субтокена
            else:
                label_id = self._process_subtoken(previous_label, previous_bio_prefix)
                aligned_labels.append(label_id)

        return aligned_labels

    def _should_ignore_token(self, token, word_id):
        """
        Проверяет, нужно ли игнорировать токен.
        
        Args:
            token: Токен для проверки
            word_id: ID слова для токена
            
        Returns:
            True если токен нужно игнорировать (пометить как -100)
        """
        punctuation_tokens = [
            ",", ".", "!", "?", ";", ":", "-",
            "(", ")", "'", '"', "``", "''"
        ]
        return (token in punctuation_tokens) or (word_id is None)

    def _process_new_word(self, word_id, original_labels):
        """
        Обработка нового слова.
        
        Args:
            word_id: ID текущего слова
            original_labels: Исходные метки для слов
            
        Returns:
            tuple: (label_id, previous_label, previous_bio_prefix)
                - label_id: ID метки для текущего токена
                - previous_label: строковое представление метки
                - previous_bio_prefix: BIO-префикс для субтокенов (I-метка)
        """
        if word_id < len(original_labels):
            label = original_labels[word_id]
            label_id = Config.LABEL2ID.get(label, Config.LABEL2ID["O"])
            
            # Определяем BIO-префикс для субтокенов
            if label.startswith("B-"):
                bio_prefix = "I-" + label[2:]
            else:
                bio_prefix = None
                
            return label_id, label, bio_prefix
        else:
            return Config.LABEL2ID["O"], "O", None

    def _process_subtoken(self, previous_label, previous_bio_prefix):
        """
        Обработка субтокена (продолжения слова).
        
        Args:
            previous_label: Метка предыдущего токена
            previous_bio_prefix: BIO-префикс для текущей сущности
            
        Returns:
            ID метки для субтокена
        """
        if previous_label and previous_label != "O" and previous_bio_prefix:
            return Config.LABEL2ID.get(previous_bio_prefix, Config.LABEL2ID["O"])
        return Config.LABEL2ID["O"]

    def _get_word_ids_fallback(self, text, token_ids) -> List[Optional[int]]:
        """
        Альтернативный метод получения word_ids для медленных токенизаторов.
        
        Args:
            text: Исходный текст (не используется, оставлен для совместимости)
            token_ids: ID токенов
            
        Returns:
            Список ID слов
        """
        # Конвертируем токены обратно в текст для анализа
        tokens = self.tokenizer.convert_ids_to_tokens(token_ids)

        word_ids = []
        word_id = -1
        in_special = False

        for _, token in enumerate(tokens):
            if token in [
                self.tokenizer.cls_token,
                self.tokenizer.sep_token,
                self.tokenizer.pad_token,
            ]:
                word_ids.append(None)
                in_special = True
            elif token.startswith("##"):
                # Субтокен - продолжение предыдущего слова
                word_ids.append(word_id)
            else:
                # Новое слово
                if not in_special:
                    word_id += 1
                word_ids.append(word_id)
                in_special = False

        return word_ids


class DataProcessor:
    @staticmethod
    def convert_to_bio_format(tokens, tags):
        """
        Конвертация тегов в BIO формат для новых категорий.
        
        Args:
            tokens: Список токенов
            tags: Список тегов без BIO-разметки
            
        Returns:
            Список тегов в BIO-формате
        """
        bio_tags = []
        prev_tag = "O"

        for _, tag in enumerate(tags):
            if tag == "O":
                bio_tags.append("O")
                prev_tag = "O"
            else:
                # Проверяем, является ли текущий тег продолжением предыдущего
                if prev_tag == tag:
                    bio_tags.append(f"I-{tag}")
                else:
                    bio_tags.append(f"B-{tag}")
                prev_tag = tag

        return bio_tags

    @staticmethod
    def validate_labels(labels):
        """
        Проверка корректности меток.
        
        Args:
            labels: Список меток для проверки
            
        Returns:
            True если все метки валидны
        """
        valid_labels = set(Config.LABEL_LIST)
        invalid_labels = set(labels) - valid_labels

        if invalid_labels:
            print(f"Warning: Found invalid labels: {invalid_labels}")
            return False
        return True

    @staticmethod
    def validate_bio_scheme(labels):
        """
        Проверка корректности BIO-схемы в датасете.
        
        Args:
            labels: Список меток для проверки
            
        Returns:
            Список ошибок (пустой если ошибок нет)
        """
        errors = []
        for i, label in enumerate(labels):
            if label.startswith("I-"):
                if i == 0 or not labels[i - 1].endswith(label[2:]):
                    errors.append(f"Position {i}: I- without preceding B-: {label}")
        return errors

    @staticmethod
    def get_statistics(texts, labels):
        """
        Получение статистики по меткам в датасете.
        
        Args:
            texts: Список текстов
            labels: Список последовательностей меток
            
        Returns:
            Словарь со статистикой по меткам
        """
        stats = {}

        for _, label_seq in zip(texts, labels):
            for label in label_seq:
                if label not in stats:
                    stats[label] = 0
                stats[label] += 1

        print("\nDataset Statistics:")
        print("-" * 40)

        # Группируем по базовым классам (без BIO префиксов)
        classes = {
            "ADVERBIAL": "Обстоятельство",
            "SUBJECT": "Подлежащее",
            "PREDICATE": "Сказуемое",
            "DEFINITION": "Определение",
            "ADDITION": "Дополнение",
        }

        for class_name, class_display in classes.items():
            b_count = stats.get(f"B-{class_name}", 0)
            i_count = stats.get(f"I-{class_name}", 0)
            if b_count > 0 or i_count > 0:
                print(f"\n{class_display}:")
                print(f"  B-{class_name}: {b_count}")
                print(f"  I-{class_name}: {i_count}")

        print(f"\nO tags: {stats.get('O', 0)}")
        print("-" * 40)

        return stats

    @staticmethod
    def split_into_sentences(text, language="russian"):
        """
        Разделение текста на предложения.
        
        Args:
            text: Исходный текст
            language: Язык для токенизации
            
        Returns:
            Список предложений
        """
        try:
            nltk.data.find("tokenizers/punkt")
        except LookupError:
            nltk.download("punkt")

        try:
            from nltk.tokenize import sent_tokenize

            return sent_tokenize(text, language=language)
        except Exception as e:
            print(f"Error in sentence tokenization: {e}")
            # Простое разделение по точкам как запасной вариант
            sentences = []
            for sent in text.split("."):
                sent = sent.strip()
                if sent:
                    sentences.append(sent + ".")
            return sentences

    @staticmethod
    def load_dataset(filepath):
        """
        Загрузка размеченного датасета.
        
        Args:
            filepath: Путь к файлу с датасетом
            
        Returns:
            Tuple:
                - список текстов
                - список последовательностей меток
                
        Raises:
            FileNotFoundError: Если файл не найден
            ValueError: Если нет валидных данных
        """
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"Dataset file not found: {filepath}")

        with open(filepath, "r", encoding="utf-8") as f:
            data = json.load(f)

        texts = []
        labels = []

        for item in data:
            tokens = item["tokens"]
            tags = item["tags"]

            if len(tokens) != len(tags):
                print(f"Warning: Mismatch in tokens/tags length in item: {item}")
                continue

            text = " ".join(tokens)
            texts.append(text)
            labels.append(tags)

        if len(texts) == 0:
            raise ValueError(f"No valid data found in {filepath}")

        print(f"Loaded {len(texts)} samples from {filepath}")
        return texts, labels

    @staticmethod
    def save_dataset(filepath, data):
        """
        Сохранение датасета в файл.
        
        Args:
            filepath: Путь для сохранения
            data: Данные для сохранения
        """
        os.makedirs(os.path.dirname(filepath), exist_ok=True)

        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

        print(f"Saved dataset to {filepath} ({len(data)} samples)")

    @staticmethod
    def prepare_sequences(sentences, max_seq_length=Config.MAX_LEN):
        """
        Подготовка последовательностей для модели.
        
        Args:
            sentences: Список предложений
            max_seq_length: Максимальная длина последовательности
            
        Returns:
            Список словарей с input_ids, attention_mask и текстом
        """
        tokenizer = AutoTokenizer.from_pretrained(Config.MODEL_NAME)

        sequences = []
        for sentence in sentences:
            encoding = tokenizer.encode_plus(
                sentence,
                add_special_tokens=True,
                max_length=max_seq_length,
                truncation=True,
                return_attention_mask=True,
            )
            sequences.append(
                {
                    "input_ids": encoding["input_ids"],
                    "attention_mask": encoding["attention_mask"],
                    "text": sentence,
                }
            )

        return sequences
