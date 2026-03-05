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

        for i, (token, pred, offset) in enumerate(zip(tokens, predictions, offsets)):
            # Пропускаем специальные токены и паддинг
            if token in [
                self.tokenizer.cls_token,
                self.tokenizer.sep_token,
                self.tokenizer.pad_token,
            ]:
                continue

            if offset[0] == offset[1] == 0:  # Специальные токены
                continue

            label = Config.ID2LABEL.get(pred, "O")

            results.append(
                {
                    "token": token,
                    "label": label,
                    "start": offset[0],
                    "end": offset[1],
                    "is_subword": token.startswith("##"),
                }
            )

        return results

    def extract_circumstances(self, text: str) -> List[Dict]:
        """Извлечение обстоятельств из текста"""
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

            all_results.append(
                {
                    "sentence": sentence,
                    "tokens": [p["token"] for p in predictions],
                    "entities": entities,
                }
            )

            # Обновляем смещение для следующего предложения
            sentence_start += len(sentence) + 1  # +1 для пробела/знака препинания

        return all_results

    def _split_into_sentences(self, text: str) -> List[str]:
        """Простое разделение на предложения"""
        # Более надежное разделение
        sentence_endings = r"(?<=[.!?])\s+"
        sentences = re.split(sentence_endings, text.strip())
        return [s.strip() for s in sentences if s.strip()]

    def _group_entities_with_offsets(
        self, predictions: List[Dict], sentence: str, sentence_offset: int = 0
    ) -> List[Dict]:
        """Группировка сущностей с использованием позиций в тексте"""
        entities = []
        current_entity = None

        for pred in predictions:
            label = pred["label"]
            start = pred["start"] + sentence_offset
            end = pred["end"] + sentence_offset
            token = pred["token"]

            if label.startswith("B-"):
                # Начало новой сущности
                if current_entity:
                    entities.append(current_entity)

                entity_type = label[2:]
                current_entity = {
                    "text": self._clean_token(token),
                    "type": entity_type,
                    "start": start,
                    "end": end,
                    "label": label,
                    "tokens": [self._clean_token(token)],
                }

            elif label.startswith("I-"):
                # Продолжение сущности
                if current_entity and label[2:] == current_entity["type"]:
                    # Проверяем, что токены идут подряд
                    if start == current_entity["end"]:
                        # Субтокен - соединяем без пробела
                        if pred["is_subword"]:
                            current_entity["text"] += self._clean_token(token)
                        else:
                            current_entity["text"] += " " + self._clean_token(token)
                        current_entity["end"] = end
                        current_entity["tokens"].append(self._clean_token(token))
                    else:
                        # Разрыв - заканчиваем текущую и начинаем новую
                        entities.append(current_entity)
                        entity_type = label[2:]
                        current_entity = {
                            "text": self._clean_token(token),
                            "type": entity_type,
                            "start": start,
                            "end": end,
                            "label": label,
                            "tokens": [self._clean_token(token)],
                        }
                else:
                    # Несоответствие меток
                    if current_entity:
                        entities.append(current_entity)
                    current_entity = None

            elif label == "O" and current_entity:
                # Конец сущности
                entities.append(current_entity)
                current_entity = None

        # Добавляем последнюю сущность
        if current_entity:
            entities.append(current_entity)

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
        return token.replace("##", "")

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
