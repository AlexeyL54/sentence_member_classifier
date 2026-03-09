import json
import torch
from torch.utils.data import Dataset
from transformers import AutoTokenizer
from config import Config
import nltk
import os


class CircumstanceDataset(Dataset):
    def __init__(self, texts, labels=None, tokenizer=None, max_len=Config.MAX_LEN):
        self.texts = texts
        self.labels = labels
        # Используем AutoTokenizer, который загружает быстрый токенизатор если доступен
        self.tokenizer = tokenizer or AutoTokenizer.from_pretrained(Config.MODEL_NAME)
        self.max_len = max_len

        # Проверяем, что токенизатор быстрый
        if not self.tokenizer.is_fast:
            print("Warning: Using slow tokenizer. Word ids may not be available.")

    def __len__(self):
        return len(self.texts)

    def __getitem__(self, idx):
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
        """Выравниваем метки с субтокенами BERT"""
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

        for i, word_id in enumerate(word_ids):
            token = tokens[i]
            
            # Проверяем, является ли токен знаком препинания
            if token in [",", ".", "!", "?", ";", ":", "-", "(", ")", "'", '"', "``", "''"]:
                aligned_labels.append(-100)
                previous_word_id = None
                previous_label = None
                previous_bio_prefix = None
                continue

            if word_id is None:
                # Специальные токены - всегда игнорируем
                aligned_labels.append(-100)
                previous_word_id = None
                previous_label = None
                previous_bio_prefix = None
            elif word_id != previous_word_id:
                # Новое слово
                if word_id < len(original_labels):
                    label = original_labels[word_id]
                    aligned_labels.append(Config.LABEL2ID.get(label, Config.LABEL2ID["O"]))
                    previous_label = label
                    # Запоминаем BIO-префикс для субтокенов
                    if label.startswith("B-"):
                        previous_bio_prefix = "I-" + label[2:]
                    else:
                        previous_bio_prefix = None
                else:
                    aligned_labels.append(Config.LABEL2ID["O"])
                    previous_label = "O"
                    previous_bio_prefix = None
                previous_word_id = word_id
            else:
                # Субтокен того же слова
                if previous_label and previous_label != "O" and previous_bio_prefix:
                    # Для субтокенов используем I- префикс
                    aligned_labels.append(Config.LABEL2ID.get(previous_bio_prefix, Config.LABEL2ID["O"]))
                else:
                    aligned_labels.append(Config.LABEL2ID["O"])

        return aligned_labels

    def _get_word_ids_fallback(self, text, token_ids):
        """Альтернативный метод получения word_ids для медленных токенизаторов"""
        # Конвертируем токены обратно в текст для анализа
        tokens = self.tokenizer.convert_ids_to_tokens(token_ids)

        word_ids = []
        word_id = -1
        in_special = False

        for i, token in enumerate(tokens):
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
        """Конвертация тегов в BIO формат для новых категорий"""
        bio_tags = []
        prev_tag = "O"

        for i, tag in enumerate(tags):
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
        """Проверка корректности меток"""
        valid_labels = set(Config.LABEL_LIST)
        invalid_labels = set(labels) - valid_labels

        if invalid_labels:
            print(f"Warning: Found invalid labels: {invalid_labels}")
            return False
        return True

    @staticmethod

    def validate_bio_scheme(labels):
        """Проверка корректности BIO-схемы в датасете"""
        errors = []
        for i, label in enumerate(labels):
            if label.startswith("I-"):
                if i == 0 or not labels[i-1].endswith(label[2:]):
                    errors.append(f"Position {i}: I- without preceding B-: {label}")
        return errors

    @staticmethod
    def get_statistics(texts, labels):
        """Получение статистики по меткам в датасете"""
        stats = {}

        for text, label_seq in zip(texts, labels):
            for label in label_seq:
                if label not in stats:
                    stats[label] = 0
                stats[label] += 1

        print("\nDataset Statistics:")
        print("-" * 40)

        # Группировка по типам
        categories = {
            "Circumstances": [
                "MANNER",
                "TIME",
                "DEGREE",
                "CONDITION",
                "CAUSE",
                "CONCESSION",
                "LOCATION",
                "PURPOSE",
            ],
            "Main Parts": ["SUBJECT", "PREDICATE"],
            "Secondary Parts": [
                "ADDITION",
                "DEFINITION",
            ],
        }

        for category, tags in categories.items():
            print(f"\n{category}:")
            for tag in tags:
                b_count = stats.get(f"B-{tag}", 0)
                i_count = stats.get(f"I-{tag}", 0)
                if b_count > 0 or i_count > 0:
                    print(f"  {tag}: B={b_count}, I={i_count}")

        print(f"\nO tags: {stats.get('O', 0)}")
        print("-" * 40)

        return stats

    @staticmethod
    def split_into_sentences(text, language="russian"):
        """Разделение текста на предложения"""
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
        """Загрузка размеченного датасета"""
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
        """Сохранение датасета в файл"""
        os.makedirs(os.path.dirname(filepath), exist_ok=True)

        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

        print(f"Saved dataset to {filepath} ({len(data)} samples)")

    @staticmethod
    def prepare_sequences(sentences, max_seq_length=Config.MAX_LEN):
        """Подготовка последовательностей для модели"""
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
