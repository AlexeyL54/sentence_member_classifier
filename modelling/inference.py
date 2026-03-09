import torch
import json
import os
import re
from typing import List, Dict
from transformers import AutoTokenizer, AutoModelForTokenClassification
from config import Config


class CircumstancePredictor:
    def __init__(self, model_path=Config.MODEL_SAVE_PATH):
        self.device = Config.DEVICE

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
        except Exception as e:
            raise RuntimeError(f"Error loading model: {e}")

    def preprocess_text(self, text: str) -> str:
        """Предобработка текста перед токенизацией"""
        # Заменяем множественные пробелы на один
        text = re.sub(r'\s+', ' ', text)
        # Убираем пробелы перед знаками препинания
        text = re.sub(r'\s+([.,!?;:])', r'\1', text)
        # Добавляем пробел после знаков препинания, если его нет (для корректного разделения на предложения)
        text = re.sub(r'([.,!?;:])([^\s])', r'\1 \2', text)
        return text.strip()

    def tokenize_with_offsets(self, text: str):
        """Токенизация с сохранением позиций в исходном тексте"""
        encoding = self.tokenizer(
            text,
            add_special_tokens=True,
            max_length=Config.MAX_LEN,
            padding="max_length",
            truncation=True,
            return_attention_mask=True,
            return_tensors="pt",
            return_offsets_mapping=True,  # Важно для группировки!
        )
        return encoding

    def predict_sentence(self, sentence: str) -> List[Dict]:
        """Предсказание для одного предложения с правильной обработкой"""
        # Токенизация с offsets
        encoding = self.tokenize_with_offsets(sentence)

        input_ids = encoding["input_ids"].to(self.device)
        attention_mask = encoding["attention_mask"].to(self.device)
        offsets = encoding["offset_mapping"][0].cpu().numpy()

        # Предсказание
        with torch.no_grad():
            outputs = self.model(input_ids=input_ids, attention_mask=attention_mask)
            logits = outputs.logits if hasattr(outputs, "logits") else outputs[0]
            predictions = torch.argmax(logits, dim=-1)

        predictions = predictions[0].cpu().numpy()

        # Обработка результатов
        results = []
        tokens = self.tokenizer.convert_ids_to_tokens(input_ids[0].cpu().numpy())

        print(f"\nDebug - Sentence: {sentence}")
        print("Token | Label | is_subword | start-end")
        
        for i, (token, pred, offset) in enumerate(zip(tokens, predictions, offsets)):
            # Пропускаем специальные токены и паддинг
            if token in [
                self.tokenizer.cls_token,
                self.tokenizer.sep_token,
                self.tokenizer.pad_token,
                self.tokenizer.unk_token,
            ]:
                continue

            if offset[0] == offset[1] == 0:  # Специальные токены
                continue

            # Проверяем, является ли токен знаком препинания
            if token in [",", ".", "!", "?", ";", ":", "-", "(", ")", "'", '"', "``", "''"]:
                # Пропускаем знаки препинания
                continue

            label = Config.ID2LABEL.get(pred, "O")
            
            # Добавляем отладочный вывод
            is_subword = token.startswith("##")
            print(f"{token:15} | {label:15} | {is_subword} | {offset[0]}-{offset[1]}")

            results.append(
                {
                    "token": token,
                    "label": label,
                    "start": offset[0],
                    "end": offset[1],
                    "is_subword": is_subword,
                }
            )

        return results

    def _label_to_type(self, label: str) -> str:
        """Преобразование метки в читаемый тип"""
        label_to_type = {
            "O": "не_является_членом_предложения",
            "B-MANNER": "обстоятельство_образа_действия",
            "I-MANNER": "обстоятельство_образа_действия",
            "B-TIME": "обстоятельство_времени",
            "I-TIME": "обстоятельство_времени",
            "B-DEGREE": "обстоятельство_степени",
            "I-DEGREE": "обстоятельство_степени",
            "B-CONDITION": "обстоятельство_условия",
            "I-CONDITION": "обстоятельство_условия",
            "B-CAUSE": "обстоятельство_причины",
            "I-CAUSE": "обстоятельство_причины",
            "B-CONCESSION": "обстоятельство_уступки",
            "I-CONCESSION": "обстоятельство_уступки",
            "B-LOCATION": "обстоятельство_места",
            "I-LOCATION": "обстоятельство_места",
            "B-PURPOSE": "обстоятельство_цели",
            "I-PURPOSE": "обстоятельство_цели",
            "B-SUBJECT": "подлежащее",
            "I-SUBJECT": "подлежащее",
            "B-PREDICATE": "сказуемое",
            "I-PREDICATE": "сказуемое",
            "B-DEFINITION": "определение",
            "I-DEFINITION": "определение",
            "B-ADDITION": "дополнение",
            "I-ADDITION": "дополнение",
        }
        return label_to_type.get(label, "неизвестно")

    def extract_circumstances(self, text: str) -> List[Dict]:
        """Извлечение всех членов предложения из текста"""
        # Предобработка текста
        text = self.preprocess_text(text)
        
        sentences = self._split_into_sentences(text)

        all_results = []
        sentence_start = 0

        for sentence in sentences:
            # Предсказание для предложения
            predictions = self.predict_sentence(sentence)

            # Группировка в сущности с учетом смещения
            entities = self._group_entities_with_offsets(
                predictions, sentence, sentence_start
            )

            # Также создаем список всех токенов с их классификацией
            all_tokens = []
            for pred in predictions:
                token_info = {
                    "text": self._clean_token(pred["token"]),
                    "label": pred["label"],
                    "type": self._label_to_type(pred["label"]),
                    "start": pred["start"] + sentence_start,
                    "end": pred["end"] + sentence_start,
                    "is_subword": pred["is_subword"],
                }
                all_tokens.append(token_info)

            all_results.append(
                {
                    "sentence": sentence,
                    "tokens": all_tokens,  # Все токены с классификацией
                    "entities": entities,  # Сгруппированные сущности (только не-O)
                }
            )

            # Обновляем смещение для следующего предложения
            sentence_start += len(sentence) + 1

        return all_results

    def _split_into_sentences(self, text: str) -> List[str]:
        """Простое разделение на предложения"""
        # Более надежное разделение
        sentence_endings = r"(?<=[.!?])\s+"
        sentences = re.split(sentence_endings, text.strip())
        return [s.strip() for s in sentences if s.strip()]

    def analyze_sentence_structure(self, text: str) -> Dict:
        """Полный анализ структуры предложения"""
        results = self.extract_circumstances(text)

        if not results:
            return {}

        sentence_data = results[0]

        # Классифицируем найденные сущности
        structure = {
            "sentence": sentence_data["sentence"],
            "main_parts": {"subject": [], "predicate": []},
            "secondary_parts": {"addition": [], "definition": []},
            "circumstances": [],
        }

        for entity in sentence_data.get("entities", []):
            entity_type = entity["type"].lower()

            if entity_type == "subject":
                structure["main_parts"]["subject"].append(entity)
            elif entity_type == "predicate":
                structure["main_parts"]["predicate"].append(entity)
            elif entity_type == "addition":
                structure["secondary_parts"]["addition"].append(entity)
            elif entity_type == "definition":
                structure["secondary_parts"]["definition"].append(entity)
            elif entity_type in [
                "manner",
                "time",
                "location",
                "cause",
                "purpose",
                "condition",
                "concession",
                "degree",
            ]:
                structure["circumstances"].append(entity)

        return structure

    def _group_entities_with_offsets(
    self, predictions: List[Dict], sentence: str, sentence_offset: int = 0
) -> List[Dict]:
        """Группировка сущностей с использованием позиций в тексте"""
        entities = []
        
        # Карта для преобразования меток в читаемые типы
        label_to_type = {
            "O": "не_является_членом_предложения",
            "MANNER": "обстоятельство_образа_действия",
            "TIME": "обстоятельство_времени",
            "DEGREE": "обстоятельство_степени",
            "CONDITION": "обстоятельство_условия",
            "CAUSE": "обстоятельство_причины",
            "CONCESSION": "обстоятельство_уступки",
            "LOCATION": "обстоятельство_места",
            "PURPOSE": "обстоятельство_цели",
            "SUBJECT": "подлежащее",
            "PREDICATE": "сказуемое",
            "DEFINITION": "определение",
            "ADDITION": "дополнение",
        }

        i = 0
        while i < len(predictions):
            pred = predictions[i]
            label = pred["label"]
            
            # Пропускаем O-метки
            if label == "O":
                i += 1
                continue
                
            if label.startswith("B-"):
                # Начало новой сущности
                entity_type = label[2:]
                start = pred["start"] + sentence_offset
                end = pred["end"] + sentence_offset
                
                # Начинаем с первого токена
                entity_text = self._clean_token(pred["token"])
                entity_tokens = [entity_text]
                
                # Смотрим следующие токены с I- той же сущности
                j = i + 1
                while j < len(predictions):
                    next_pred = predictions[j]
                    next_label = next_pred["label"]
                    
                    # Если это I- той же сущности
                    if next_label.startswith("I-") and next_label[2:] == entity_type:
                        clean_token = self._clean_token(next_pred["token"])
                        entity_tokens.append(clean_token)
                        
                        # Проверяем, является ли токен субтокеном
                        if next_pred["is_subword"]:
                            entity_text += clean_token
                        else:
                            entity_text += " " + clean_token
                        
                        end = next_pred["end"] + sentence_offset
                        j += 1
                    else:
                        break
                
                # Создаем сущность
                entity = {
                    "text": entity_text,
                    "type": label_to_type.get(entity_type, entity_type.lower()),
                    "original_type": entity_type,
                    "start": start,
                    "end": end,
                    "label": label,
                    "tokens": entity_tokens,
                }
                
                entities.append(entity)
                i = j  # Продолжаем с последнего необработанного токена
            else:
                # Если встретили I- без B- (ошибка модели), пропускаем
                i += 1

        return entities

    def _clean_token(self, token: str) -> str:
        """Очистка токена от специальных символов"""
        if token.startswith("##"):
            return token[2:]
        if token in [
            self.tokenizer.cls_token,
            self.tokenizer.sep_token,
            self.tokenizer.pad_token,
            self.tokenizer.unk_token,
        ]:
            return ""
        return token

    def process_file(self, input_file: str, output_file: str = "") -> Dict:
        """Обработка текстового файла"""
        if not os.path.exists(input_file):
            raise FileNotFoundError(f"Input file not found: {input_file}")

        print(f"Processing file: {input_file}")

        # Чтение файла
        with open(input_file, "r", encoding="utf-8") as f:
            text = f.read()

        # Извлечение обстоятельств
        results = self.extract_circumstances(text)

        # Формирование результата
        output_data = {
            "input_file": input_file,
            "total_sentences": len(results),
            "total_circumstances": sum(len(r["entities"]) for r in results),
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
        print(f"Found {output_data['total_circumstances']} circumstances")

        return output_data
