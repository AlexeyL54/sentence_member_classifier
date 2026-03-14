import torch
import json
import os
import re
from typing import List, Dict, Tuple
from transformers import AutoTokenizer, AutoModelForTokenClassification
from config import Config


class CircumstancePredictor:
    def __init__(self, model_path=Config.MODEL_SAVE_PATH, debug=False):
        self.device = Config.DEVICE
        self.debug = debug  # Флаг для отладочного вывода

        # Загрузка токенизатора
        tokenizer_path = os.path.join(model_path, "tokenizer")
        if os.path.exists(tokenizer_path):
            self.tokenizer = AutoTokenizer.from_pretrained(tokenizer_path)
        else:
            self.tokenizer = AutoTokenizer.from_pretrained(Config.MODEL_NAME)

        # Загрузка модели
        if not os.path.exists(model_path):
            raise FileNotFoundError(
                f"Model not found at {model_path}. "
                f"Please train the model first with: python main.py --train"
            )

        try:
            # Загружаем модель
            self.model = AutoModelForTokenClassification.from_pretrained(
                model_path, num_labels=Config.NUM_LABELS
            )
            self.model.to(self.device)
            self.model.eval()
            print(f"✓ Model loaded from {model_path}")

            # Загружаем метаданные для маппинга меток
            self.load_metadata(model_path)

        except Exception as e:
            raise RuntimeError(f"Error loading model: {e}")

    def load_metadata(self, model_path):
        """Загрузка метаданных модели"""
        metadata_path = os.path.join(model_path, "metadata.json")
        if os.path.exists(metadata_path):
            with open(metadata_path, "r", encoding="utf-8") as f:
                self.metadata = json.load(f)
        else:
            self.metadata = None

    def preprocess_text(self, text: str) -> str:
        """Предобработка текста перед токенизацией"""
        # Заменяем множественные пробелы на один
        text = re.sub(r"\s+", " ", text)
        # Убираем пробелы перед знаками препинания
        text = re.sub(r"\s+([.,!?;:])", r"\1", text)
        return text.strip()

    def predict_sentence(
        self, sentence: str
    ) -> Tuple[List[Dict], List[Tuple[int, int]]]:
        """
        Предсказание для одного предложения

        Returns:
            - список токенов с предсказаниями
            - список позиций (start, end) для каждого токена в исходном тексте
        """
        # Токенизация с offsets
        encoding = self.tokenizer(
            sentence,
            add_special_tokens=True,
            max_length=Config.MAX_LEN,
            padding="max_length",
            truncation=True,
            return_attention_mask=True,
            return_tensors="pt",
            return_offsets_mapping=True,
        )

        input_ids = encoding["input_ids"].to(self.device)
        attention_mask = encoding["attention_mask"].to(self.device)
        offset_mapping = encoding["offset_mapping"][0].cpu().numpy()

        # Предсказание
        with torch.no_grad():
            outputs = self.model(input_ids=input_ids, attention_mask=attention_mask)
            logits = outputs.logits if hasattr(outputs, "logits") else outputs[0]

            # Если есть CRF, используем его для декодирования
            if hasattr(self.model, "crf") and self.model.crf is not None:
                mask = attention_mask.bool()
                predictions = self.model.crf.decode(logits, mask=mask)
                predictions = torch.tensor(predictions)[0]
            else:
                predictions = torch.argmax(logits, dim=-1)[0]

        predictions = predictions.cpu().numpy()
        tokens = self.tokenizer.convert_ids_to_tokens(input_ids[0].cpu().numpy())

        # Собираем результаты
        results = []
        valid_offsets = []

        # Отладочный вывод
        if self.debug:
            print(f"\nDebug - Sentence: {sentence}")
            print(f"{'Token':15} | {'Label':15} | {'is_subword':10} | {'start-end':10}")
            print("-" * 60)

        for i, (token, pred, offset) in enumerate(
            zip(tokens, predictions, offset_mapping)
        ):
            # Пропускаем специальные токены
            if token in [
                self.tokenizer.cls_token,
                self.tokenizer.sep_token,
                self.tokenizer.pad_token,
                self.tokenizer.unk_token,
            ]:
                continue

            # Пропускаем токены с нулевым смещением (padding)
            if offset[0] == offset[1] == 0:
                continue

            label = Config.ID2LABEL.get(pred, "O")
            is_subword = token.startswith("##")

            # Для субтокенов убираем ##
            clean_token = token[2:] if is_subword else token

            # Отладочный вывод
            if self.debug:
                print(
                    f"{token:15} | {label:15} | {str(is_subword):10} | {offset[0]}-{offset[1]}"
                )

            results.append(
                {
                    "token": clean_token,
                    "full_token": token,
                    "label": label,
                    "is_subword": is_subword,
                    "position": i,
                }
            )
            valid_offsets.append((offset[0], offset[1]))

        if self.debug:
            print("-" * 60)

        return results, valid_offsets

    def _should_merge_words(self, word1: Dict, word2: Dict) -> bool:
        """
        Определяет, должны ли два слова быть объединены в одну сущность

        Правила BIO разметки:
        - B- (Begin) - начало новой сущности
        - I- (Inside) - продолжение текущей сущности
        - O - вне сущности

        Объединяем только если:
        1. Второе слово имеет I- префикс (продолжение)
        2. И базовый тип совпадает с первым словом
        3. И расстояние между словами минимально (только пробелы)
        """
        if not word1 or not word2:
            return False

        label1 = word1["main_label"]
        label2 = word2["main_label"]

        # Получаем базовые типы без BIO префиксов
        base1 = label1[2:] if label1.startswith(("B-", "I-")) else label1
        base2 = label2[2:] if label2.startswith(("B-", "I-")) else label2

        # Проверяем расстояние между словами (в символах)
        distance = word2["start"] - word1["end"]

        # Объединяем ТОЛЬКО если:
        # 1. Второе слово имеет I- префикс (продолжение текущей сущности)
        # 2. Базовые типы совпадают
        # 3. Расстояние небольшое (только пробелы, не более 1-2 пробелов)
        # 4. Не O-метки
        return (
            label2.startswith("I-")  # Ключевое изменение: только I- префикс!
            and base1 == base2
            and base1 != "O"
            and distance < 3  # максимум 2 пробела
        )

    def _group_into_phrases(self, words: List[Dict]) -> List[Dict]:
        """
        Группировка слов в фразы (составные члены предложения)
        с соблюдением BIO разметки
        """
        if not words:
            return []

        phrases = []
        current_phrase = None

        for word in words:
            # Пропускаем знаки препинания как отдельные слова
            if word["text"] in [",", ".", "!", "?", ";", ":", "-", "(", ")"]:
                continue

            label = word["main_label"]

            # Если это начало новой сущности (B- префикс) или первое слово
            if label.startswith("B-") or current_phrase is None:
                # Завершаем предыдущую фразу, если она есть
                if current_phrase is not None:
                    phrases.append(current_phrase)

                # Начинаем новую фразу
                current_phrase = word.copy()
                current_phrase["text"] = word["text"]
                current_phrase["words"] = [word]

            # Если это продолжение сущности (I- префикс) и есть текущая фраза
            elif label.startswith("I-") and current_phrase is not None:
                # Проверяем, что базовый тип совпадает с текущей фразой
                current_base = (
                    current_phrase["main_label"][2:]
                    if current_phrase["main_label"].startswith(("B-", "I-"))
                    else current_phrase["main_label"]
                )
                word_base = label[2:] if label.startswith(("B-", "I-")) else label

                if current_base == word_base and self._should_merge_words(
                    current_phrase, word
                ):
                    # Продолжаем текущую фразу
                    current_phrase["text"] += " " + word["text"]
                    current_phrase["end"] = word["end"]
                    current_phrase["words"].append(word)
                else:
                    # Если типы не совпадают, начинаем новую фразу
                    phrases.append(current_phrase)
                    current_phrase = word.copy()
                    current_phrase["text"] = word["text"]
                    current_phrase["words"] = [word]

            else:
                # Для O-меток или других случаев
                if current_phrase is not None:
                    phrases.append(current_phrase)
                    current_phrase = None

        # Добавляем последнюю фразу
        if current_phrase is not None:
            phrases.append(current_phrase)

        return phrases

    def _group_into_words(
        self, tokens: List[Dict], offsets: List[Tuple[int, int]]
    ) -> List[Dict]:
        """
        Группировка субтокенов в полные слова с сохранением BIO-разметки
        """
        words = []
        current_word = None
        current_start = None
        current_end = None
        current_labels = []
        current_tokens = []

        for i, (token_info, (start, end)) in enumerate(zip(tokens, offsets)):
            if token_info["is_subword"] and current_word is not None:
                # Субтокен - добавляем к текущему слову
                current_word += token_info["token"]
                current_end = end
                current_labels.append(token_info["label"])
                current_tokens.append(token_info)
            else:
                # Новое слово
                if current_word is not None:
                    # Сохраняем предыдущее слово
                    main_label = self._get_main_label(current_labels)
                    words.append(
                        {
                            "text": current_word,
                            "start": current_start,
                            "end": current_end,
                            "labels": current_labels,
                            "main_label": main_label,
                            "has_b_prefix": any(
                                l.startswith("B-") for l in current_labels
                            ),  # Важно для определения начала
                            "tokens": current_tokens,
                        }
                    )

                # Начинаем новое слово
                current_word = token_info["token"]
                current_start = start
                current_end = end
                current_labels = [token_info["label"]]
                current_tokens = [token_info]

        # Добавляем последнее слово
        if current_word is not None:
            main_label = self._get_main_label(current_labels)
            words.append(
                {
                    "text": current_word,
                    "start": current_start,
                    "end": current_end,
                    "labels": current_labels,
                    "main_label": main_label,
                    "has_b_prefix": any(l.startswith("B-") for l in current_labels),
                    "tokens": current_tokens,
                }
            )

        return words

    def _get_main_label(self, labels: List[str]) -> str:
        """
        Определение основной метки для слова на основе меток его токенов

        BIO схема: если есть B-*, используем его, иначе первый I-* или O
        """
        for label in labels:
            if label.startswith("B-"):
                return label
        for label in labels:
            if label.startswith("I-"):
                return label
        return labels[0] if labels else "O"

    def _label_to_russian(self, label: str) -> str:
        """Преобразование метки в русское название"""

        # Убираем BIO префикс
        base_label = label[2:] if label.startswith(("B-", "I-")) else label

        # Маппинг на русский
        mapping = {
            "O": "не является членом предложения",
            "ADVERBIAL": "обстоятельство",
            "SUBJECT": "подлежащее",
            "PREDICATE": "сказуемое",
            "DEFINITION": "определение",
            "ADDITION": "дополнение",
        }

        return mapping.get(base_label, base_label.lower())

    def analyze_sentence(self, text: str) -> Dict:
        """Анализ одного предложения"""
        # Получаем предсказания для токенов
        tokens, offsets = self.predict_sentence(text)

        # Группируем токены в слова
        words = self._group_into_words(tokens, offsets)

        # Группируем слова в фразы
        phrases = self._group_into_phrases(words)

        # Формируем структуру предложения
        structure = {
            "sentence": text,
            "subject": [],
            "predicate": [],
            "adverbial": [],
            "addition": [],
            "definition": [],
            "phrases": phrases,  # все фразы с деталями
        }

        # Распределяем по категориям
        for phrase in phrases:
            label = phrase["main_label"]
            russian_type = self._label_to_russian(label)

            if russian_type is None:  # O-метки пропускаем
                continue

            phrase_info = {
                "text": phrase["text"],
                "type": russian_type,
                "start": phrase["start"],
                "end": phrase["end"],
                "label": label,
            }

            # Добавляем в соответствующую категорию
            if russian_type == "подлежащее":
                structure["subject"].append(phrase_info)
            elif russian_type == "сказуемое":
                structure["predicate"].append(phrase_info)
            elif russian_type == "обстоятельство":
                structure["adverbial"].append(phrase_info)
            elif russian_type == "дополнение":
                structure["addition"].append(phrase_info)
            elif russian_type == "определение":
                structure["definition"].append(phrase_info)

        return structure

    def analyze_sentence_structure(self, text: str) -> Dict:
        """Анализ структуры предложения (для совместимости)"""
        return self.analyze_sentence(text)

    def extract_circumstances(self, text: str) -> List[Dict]:
        """Извлечение всех членов предложения из текста (для совместимости)"""
        # Предобработка текста
        text = self.preprocess_text(text)

        # Разделяем на предложения
        sentences = self._split_into_sentences(text)

        all_results = []

        for sentence in sentences:
            structure = self.analyze_sentence(sentence)

            # Собираем все найденные члены предложения
            entities = []
            for category in [
                "subject",
                "predicate",
                "adverbial",
                "addition",
                "definition",
            ]:
                for item in structure[category]:
                    entities.append(
                        {
                            "text": item["text"],
                            "type": item["type"],
                        }
                    )

            all_results.append(
                {
                    "sentence": sentence,
                    "entities": entities,
                }
            )

        return all_results

    def _split_into_sentences(self, text: str) -> List[str]:
        """Разделение на предложения"""
        sentences = []
        current = []

        for char in text:
            current.append(char)
            if char in ".!?":
                sentences.append("".join(current).strip())
                current = []

        # Добавляем последнее предложение, если оно не закончилось знаком препинания
        if current:
            sentences.append("".join(current).strip())

        return [s for s in sentences if s]

    def process_file(self, input_file: str, output_file: str = "") -> Dict:
        """Обработка текстового файла"""
        if not os.path.exists(input_file):
            raise FileNotFoundError(f"Input file not found: {input_file}")

        print(f"Processing file: {input_file}")

        # Чтение файла
        with open(input_file, "r", encoding="utf-8") as f:
            text = f.read()

        # Извлечение членов предложения
        results = self.extract_circumstances(text)

        # Формирование результата
        output_data = {
            "input_file": input_file,
            "total_sentences": len(results),
            "total_entities": sum(len(r["entities"]) for r in results),
            "results": results,
        }

        # Сохранение
        if output_file:
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            with open(output_file, "w", encoding="utf-8") as f:
                json.dump(output_data, f, ensure_ascii=False, indent=2)
            print(f"✓ Results saved to: {output_file}")

        # Статистика
        print(f"\nProcessed {len(results)} sentences")
        print(f"Found {output_data['total_entities']} entities")

        return output_data
