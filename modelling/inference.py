import torch
import json
import os
import re
from typing import List, Dict, Tuple, Optional, Any
from dataclasses import dataclass, field
from transformers import AutoTokenizer, AutoModelForTokenClassification
from config import Config


@dataclass
class TokenInfo:
    """Информация о токене"""
    token: str
    full_token: str
    label: str
    is_subword: bool
    position: int


@dataclass
class WordInfo:
    """Информация о слове, состоящем из одного или нескольких токенов"""
    text: str
    start: int
    end: int
    labels: List[str] = field(default_factory=list)
    main_label: str = "O"
    has_b_prefix: bool = False
    tokens: List[TokenInfo] = field(default_factory=list)


@dataclass
class PhraseInfo:
    """Информация о фразе (составном члене предложения)"""
    text: str
    start: int
    end: int
    main_label: str
    words: List[WordInfo] = field(default_factory=list)


class CircumstancePredictor:
    """
    Класс для предсказания членов предложения (обстоятельств, подлежащих, сказуемых и т.д.)
    на основе обученной BERT-модели с BIO-разметкой.
    
    Attributes:
        device (torch.device): Устройство для вычислений
        debug (bool): Флаг отладочного режима
        tokenizer (AutoTokenizer): Токенизатор BERT
        model (AutoModelForTokenClassification): Модель для классификации токенов
        metadata (dict): Метаданные модели
    """
    
    def __init__(self, model_path: str = Config.MODEL_SAVE_PATH, debug: bool = False):
        """
        Инициализация предсказателя.
        
        Args:
            model_path: Путь к директории с сохраненной моделью
            debug: Включение отладочного режима
            
        Raises:
            FileNotFoundError: Если модель не найдена
            RuntimeError: При ошибке загрузки модели
        """
        self.device = Config.DEVICE
        self.debug = debug

        self._load_tokenizer(model_path)
        self._load_model(model_path)

    def _load_tokenizer(self, model_path: str) -> None:
        """
        Загрузка токенизатора.
        
        Args:
            model_path: Путь к модели
        """
        tokenizer_path = os.path.join(model_path, "tokenizer")
        if os.path.exists(tokenizer_path):
            self.tokenizer = AutoTokenizer.from_pretrained(tokenizer_path)
        else:
            self.tokenizer = AutoTokenizer.from_pretrained(Config.MODEL_NAME)

    def _load_model(self, model_path: str) -> None:
        """
        Загрузка модели и метаданных.
        
        Args:
            model_path: Путь к модели
            
        Raises:
            FileNotFoundError: Если модель не найдена
            RuntimeError: При ошибке загрузки
        """
        if not os.path.exists(model_path):
            raise FileNotFoundError(
                f"Model not found at {model_path}. "
                f"Please train the model first with: python main.py --train"
            )

        try:
            self.model = AutoModelForTokenClassification.from_pretrained(
                model_path, num_labels=Config.NUM_LABELS
            )
            self.model.to(self.device)
            self.model.eval()
            print(f"✓ Model loaded from {model_path}")

            self._load_metadata(model_path)

        except Exception as e:
            raise RuntimeError(f"Error loading model: {e}")

    def _load_metadata(self, model_path: str) -> None:
        """
        Загрузка метаданных модели.
        
        Args:
            model_path: Путь к модели
        """
        metadata_path = os.path.join(model_path, "metadata.json")
        if os.path.exists(metadata_path):
            with open(metadata_path, "r", encoding="utf-8") as f:
                self.metadata = json.load(f)
        else:
            self.metadata = None

    @staticmethod
    def preprocess_text(text: str) -> str:
        """
        Предобработка текста перед токенизацией.
        
        Args:
            text: Исходный текст
            
        Returns:
            Обработанный текст
        """
        # Заменяем множественные пробелы на один
        text = re.sub(r"\s+", " ", text)
        # Убираем пробелы перед знаками препинания
        text = re.sub(r"\s+([.,!?;:])", r"\1", text)
        return text.strip()

    def _encode_sentence(self, sentence: str) -> Dict[str, Any]:
        """
        Токенизация предложения с подготовкой для модели.
        
        Args:
            sentence: Исходное предложение
            
        Returns:
            Словарь с закодированными данными
        """
        return self.tokenizer(
            sentence,
            add_special_tokens=True,
            max_length=Config.MAX_LEN,
            padding="max_length",
            truncation=True,
            return_attention_mask=True,
            return_tensors="pt",
            return_offsets_mapping=True,
        )

    def _get_predictions(self, input_ids: torch.Tensor, attention_mask: torch.Tensor) -> torch.Tensor:
        """
        Получение предсказаний от модели.
        
        Args:
            input_ids: ID токенов
            attention_mask: Маска внимания
            
        Returns:
            Тензор с предсказаниями
        """
        with torch.no_grad():
            outputs = self.model(input_ids=input_ids, attention_mask=attention_mask)
            logits = outputs.logits if hasattr(outputs, "logits") else outputs[0]

            if hasattr(self.model, "crf") and self.model.crf is not None:
                mask = attention_mask.bool()
                predictions = self.model.crf.decode(logits, mask=mask)
                return torch.tensor(predictions)[0]
            
            return torch.argmax(logits, dim=-1)[0]

    def _is_special_token(self, token: str) -> bool:
        """
        Проверка, является ли токен специальным.
        
        Args:
            token: Токен для проверки
            
        Returns:
            True если токен специальный
        """
        special_tokens = [
            self.tokenizer.cls_token,
            self.tokenizer.sep_token,
            self.tokenizer.pad_token,
            self.tokenizer.unk_token,
        ]
        return token in special_tokens

    def _create_token_info(self, token: str, pred: int, offset: Tuple[int, int], idx: int) -> Optional[TokenInfo]:
        """
        Создание информации о токене.
        
        Args:
            token: Токен
            pred: ID предсказанной метки
            offset: Смещение токена в тексте
            idx: Позиция токена
            
        Returns:
            Информация о токене или None для невалидных токенов
        """
        if self._is_special_token(token):
            return None
            
        if offset[0] == offset[1] == 0:
            return None

        label = Config.ID2LABEL.get(pred, "O")
        is_subword = token.startswith("##")
        clean_token = token[2:] if is_subword else token

        return TokenInfo(
            token=clean_token,
            full_token=token,
            label=label,
            is_subword=is_subword,
            position=idx
        )

    def _debug_print_tokens(self, sentence: str, tokens: List[str], 
                        predictions: List[int], offsets: List[Tuple[int, int]]) -> None:
        """
        Отладочный вывод информации о токенах.
        
        Args:
            sentence: Исходное предложение
            tokens: Список токенов
            predictions: Список предсказаний (ID меток)
            offsets: Список смещений
        """
        if not self.debug:
            return
            
        print(f"\nDebug - Sentence: {sentence}")
        print(f"{'Token':15} | {'Label':15} | {'is_subword':10} | {'start-end':10}")
        print("-" * 60)
        
        for token, pred, offset in zip(tokens, predictions, offsets):
            if self._is_special_token(token) or offset[0] == offset[1] == 0:
                continue
                
            label = Config.ID2LABEL.get(pred, "O")
            is_subword = token.startswith("##")
            
            print(f"{token:15} | {label:15} | {str(is_subword):10} | {offset[0]}-{offset[1]}")
        
        print("-" * 60)

    def predict_sentence(self, sentence: str) -> Tuple[List[TokenInfo], List[Tuple[int, int]]]:
        """
        Предсказание для одного предложения.
        
        Args:
            sentence: Исходное предложение
            
        Returns:
            Tuple:
                - список токенов с предсказаниями
                - список позиций (start, end) для каждого токена в исходном тексте
        """
        # Получаем кодировку предложения
        encoding = self._encode_sentence(sentence)
        
        # Извлекаем данные из кодировки
        input_ids = encoding["input_ids"].to(self.device)
        attention_mask = encoding["attention_mask"].to(self.device)
        offset_mapping = encoding["offset_mapping"][0].cpu().numpy()
        
        # Получаем предсказания от модели
        predictions = self._get_predictions(input_ids, attention_mask)
        predictions_np = predictions.cpu().numpy()  # Преобразуем в numpy для отладки
        predictions_list = predictions_np.tolist()  # Преобразуем в список для единообразия
        tokens = self.tokenizer.convert_ids_to_tokens(input_ids[0].cpu().numpy())
        
        # Отладочный вывод - используем predictions_list вместо predictions_np
        self._debug_print_tokens(sentence, tokens, predictions_list, offset_mapping)
        
        results = []
        valid_offsets = []
        
        for i, (token, pred, offset) in enumerate(zip(tokens, predictions_np, offset_mapping)):
            token_info = self._create_token_info(token, int(pred), offset, i)  # Преобразуем pred в int
            if token_info is None:
                continue
                
            results.append(token_info)
            valid_offsets.append((int(offset[0]), int(offset[1])))  # Преобразуем в int для безопасности
        
        return results, valid_offsets

    def _should_merge_words(self, word1: WordInfo, word2: WordInfo) -> bool:
        """
        Определяет, должны ли два слова быть объединены в одну сущность.
        
        Args:
            word1: Первое слово
            word2: Второе слово
            
        Returns:
            True если слова нужно объединить
        """
        if not word1 or not word2:
            return False

        label1 = word1.main_label
        label2 = word2.main_label

        base1 = self._get_base_label(label1)
        base2 = self._get_base_label(label2)

        distance = word2.start - word1.end

        return (
            label2.startswith("I-")
            and base1 == base2
            and base1 != "O"
            and distance < 3
        )

    @staticmethod
    def _get_base_label(label: str) -> str:
        """
        Получение базовой метки без BIO-префикса.
        
        Args:
            label: Исходная метка
            
        Returns:
            Базовая метка
        """
        return label[2:] if label.startswith(("B-", "I-")) else label

    def _process_word_for_phrase(self, word: WordInfo, current_phrase: Optional[PhraseInfo]) -> Optional[PhraseInfo]:
        """
        Обработка слова для добавления в текущую фразу или создания новой.
        
        Args:
            word: Информация о слове
            current_phrase: Текущая фраза или None
            
        Returns:
            Обновленная текущая фраза или новая фраза
        """
        label = word.main_label
        base_label = self._get_base_label(label)
        
        # Случай 1: Нет текущей фразы
        if current_phrase is None:
            if base_label != "O":
                return self._start_new_phrase(word)
            return None
        
        # Случай 2: Есть текущая фраза
        current_base = self._get_base_label(current_phrase.main_label)
        
        # Если слово не относится к сущности (O)
        if base_label == "O":
            return None  # Завершаем текущую фразу
        
        # Если слово с B-префиксом (начало новой сущности)
        if label.startswith("B-"):
            # Завершаем текущую фразу и начинаем новую
            return None  # Сигнал для внешнего кода, что нужно завершить фразу и начать новую
        
        # Если слово с I-префиксом (продолжение)
        if label.startswith("I-"):
            # Проверяем, относится ли к той же сущности
            if base_label == current_base:
                last_word = current_phrase.words[-1]
                distance = word.start - last_word.end
                
                # Если расстояние небольшое, добавляем к текущей фразе
                if distance <= 2:
                    return self._continue_phrase(word, current_phrase)
        
        # Во всех остальных случаях завершаем текущую фразу
        return None

    def _start_new_phrase(self, word: WordInfo) -> PhraseInfo:
        """
        Начало новой фразы.
        
        Args:
            word: Информация о слове
            
        Returns:
            Новая фраза
        """
        return PhraseInfo(
            text=word.text,
            start=word.start,
            end=word.end,
            main_label=word.main_label,
            words=[word]
        )

    def _continue_phrase(self, word: WordInfo, current_phrase: PhraseInfo) -> Optional[PhraseInfo]:
        """
        Продолжение текущей фразы.
        
        Args:
            word: Информация о слове
            current_phrase: Текущая фраза
            
        Returns:
            Обновленная фраза или None если нельзя продолжить
        """
        # Получаем базовые метки для сравнения
        current_base = self._get_base_label(current_phrase.main_label)
        word_base = self._get_base_label(word.main_label)
        
        # Получаем последнее слово в текущей фразе
        last_word = current_phrase.words[-1] if current_phrase.words else None
        
        # Проверяем возможность объединения
        if current_base == word_base and last_word and self._should_merge_words(last_word, word):
            # Обновляем фразу
            current_phrase.text += " " + word.text
            current_phrase.end = word.end
            current_phrase.words.append(word)
            return current_phrase
        
        return None

    def _group_into_phrases(self, words: List[WordInfo]) -> List[PhraseInfo]:
        """
        Группировка слов в фразы (составные члены предложения) на основе BIO-разметки.
        
        Args:
            words (List[WordInfo]): Список объектов WordInfo, каждый из которых содержит:
                - text: текст слова
                - start: начальная позиция в исходном тексте
                - end: конечная позиция в исходном тексте
                - main_label: основная BIO-метка слова (B-X, I-X или O)
                - labels: список меток для всех субтокенов слова
                - tokens: список субтокенов
                - has_b_prefix: флаг наличия B-префикса
        
        Returns:
            List[PhraseInfo]: Список сгруппированных фраз, где каждая фраза содержит:
                - text: полный текст фразы
                - start: начальная позиция фразы в тексте
                - end: конечная позиция фразы в тексте
                - main_label: BIO-метка фразы (обычно от первого слова с B-)
                - words: список слов, входящих во фразу
        """    
        if not words:
            return []

        phrases = []
        current_phrase = None

        for word in words:
            if self._is_punctuation(word.text):
                continue

            result = self._process_word_for_phrase(word, current_phrase)
            
            if result is None:
                # Завершаем текущую фразу, если она есть
                if current_phrase is not None:
                    phrases.append(current_phrase)
                    current_phrase = None
                
                # Проверяем, нужно ли начать новую фразу с текущего слова
                if word.main_label != "O":
                    current_phrase = self._start_new_phrase(word)
            else:
                # Продолжаем текущую фразу
                current_phrase = result

        # Добавляем последнюю фразу
        if current_phrase is not None:
            phrases.append(current_phrase)

        return phrases

    @staticmethod
    def _is_punctuation(text: str) -> bool:
        """
        Проверка, является ли текст знаком препинания.
        
        Args:
            text: Текст для проверки
            
        Returns:
            True если это знак препинания
        """
        punctuation = {",", ".", "!", "?", ";", ":", "-", "(", ")"}
        return text in punctuation

    def _group_into_words(self, tokens: List[TokenInfo], offsets: List[Tuple[int, int]]) -> List[WordInfo]:
        """
        Группировка субтокенов в полные слова.
        
        Args:
            tokens: Список токенов
            offsets: Список смещений
            
        Returns:
            Список слов
        """
        words = []
        current_word_data = self._init_word_data()
        
        for token_info, (start, end) in zip(tokens, offsets):
            if token_info.is_subword and current_word_data["current_word"] is not None:
                self._extend_current_word(current_word_data, token_info, end)
            else:
                self._finalize_current_word(words, current_word_data)
                self._start_new_word(current_word_data, token_info, start, end)

        self._finalize_current_word(words, current_word_data)
        return words

    def _init_word_data(self) -> Dict:
        """
        Инициализация данных для текущего слова.
        
        Returns:
            Словарь с данными для слова
        """
        return {
            "current_word": None,
            "current_start": None,
            "current_end": None,
            "current_labels": [],
            "current_tokens": []
        }

    def _extend_current_word(self, word_data: Dict, token_info: TokenInfo, end: int) -> None:
        """
        Расширение текущего слова новым токеном.
        
        Args:
            word_data: Данные текущего слова
            token_info: Информация о токене
            end: Конечная позиция
        """
        word_data["current_word"] += token_info.token
        word_data["current_end"] = end
        word_data["current_labels"].append(token_info.label)
        word_data["current_tokens"].append(token_info)

    def _start_new_word(self, word_data: Dict, token_info: TokenInfo, start: int, end: int) -> None:
        """
        Начало нового слова.
        
        Args:
            word_data: Данные для нового слова
            token_info: Информация о токене
            start: Начальная позиция
            end: Конечная позиция
        """
        word_data["current_word"] = token_info.token
        word_data["current_start"] = start
        word_data["current_end"] = end
        word_data["current_labels"] = [token_info.label]
        word_data["current_tokens"] = [token_info]

    def _finalize_current_word(self, words: List[WordInfo], word_data: Dict) -> None:
        """
        Завершение текущего слова и добавление его в список.
        
        Args:
            words: Список слов
            word_data: Данные текущего слова
        """
        if word_data["current_word"] is not None:
            main_label = self._get_main_label(word_data["current_labels"])
            words.append(WordInfo(
                text=word_data["current_word"],
                start=word_data["current_start"],
                end=word_data["current_end"],
                labels=word_data["current_labels"],
                main_label=main_label,
                has_b_prefix=any(l.startswith("B-") for l in word_data["current_labels"]),
                tokens=word_data["current_tokens"]
            ))

    def _get_main_label(self, labels: List[str]) -> str:
        """
        Определение основной метки для слова.
        
        Args:
            labels: Список меток токенов
            
        Returns:
            Основная метка для слова
        """
        for label in labels:
            if label.startswith("B-"):
                return label
        for label in labels:
            if label.startswith("I-"):
                return label
        return labels[0] if labels else "O"

    def _label_to_russian(self, label: str) -> str:
        """
        Преобразование метки в русское название.
        
        Args:
            label: Метка в BIO-формате
            
        Returns:
            Русское название члена предложения
        """
        base_label = self._get_base_label(label)
        
        mapping = {
            "O": "не является членом предложения",
            "ADVERBIAL": "обстоятельство",
            "SUBJECT": "подлежащее",
            "PREDICATE": "сказуемое",
            "DEFINITION": "определение",
            "ADDITION": "дополнение",
        }
        
        return mapping.get(base_label, base_label.lower())

    def _add_phrase_to_structure(self, structure: Dict, phrase: PhraseInfo) -> None:
        """
        Добавление фразы в структуру предложения.
        
        Args:
            structure: Структура предложения
            phrase: Информация о фразе
        """
        russian_type = self._label_to_russian(phrase.main_label)
        
        if russian_type == "не является членом предложения":
            return
            
        phrase_info = {
            "text": phrase.text,
            "type": russian_type,
            "start": phrase.start,
            "end": phrase.end,
            "label": phrase.main_label,
        }
        
        category_map = {
            "подлежащее": "subject",
            "сказуемое": "predicate",
            "обстоятельство": "adverbial",
            "дополнение": "addition",
            "определение": "definition",
        }
        
        category = category_map.get(russian_type)
        if category:
            structure[category].append(phrase_info)

    def analyze_sentence(self, text: str) -> Dict:
        """
        Анализ одного предложения.
        
        Args:
            text: Текст предложения
            
        Returns:
            Словарь со структурой предложения
        """
        tokens, offsets = self.predict_sentence(text)
        words = self._group_into_words(tokens, offsets)
        phrases = self._group_into_phrases(words)

        structure = {
            "sentence": text,
            "subject": [],
            "predicate": [],
            "adverbial": [],
            "addition": [],
            "definition": [],
            "phrases": [phrase.__dict__ for phrase in phrases],
        }

        for phrase in phrases:
            self._add_phrase_to_structure(structure, phrase)

        return structure

    def analyze_sentence_structure(self, text: str) -> Dict:
        """
        Анализ структуры предложения (для совместимости).
        
        Args:
            text: Текст предложения
            
        Returns:
            Словарь со структурой предложения
        """
        return self.analyze_sentence(text)

    def extract_circumstances(self, text: str) -> List[Dict]:
        """
        Извлечение всех членов предложения из текста.
        
        Args:
            text: Исходный текст
            
        Returns:
            Список результатов для каждого предложения
        """
        text = self.preprocess_text(text)
        sentences = self._split_into_sentences(text)

        all_results = []

        for sentence in sentences:
            structure = self.analyze_sentence(sentence)
            
            entities = []
            for category in ["subject", "predicate", "adverbial", "addition", "definition"]:
                for item in structure[category]:
                    entities.append({
                        "text": item["text"],
                        "type": item["type"],
                    })

            all_results.append({
                "sentence": sentence,
                "entities": entities,
            })

        return all_results

    @staticmethod
    def _split_into_sentences(text: str) -> List[str]:
        """
        Разделение текста на предложения.
        
        Args:
            text: Исходный текст
            
        Returns:
            Список предложений
        """
        sentences = []
        current = []

        for char in text:
            current.append(char)
            if char in ".!?":
                sentences.append("".join(current).strip())
                current = []

        if current:
            sentences.append("".join(current).strip())

        return [s for s in sentences if s]

    def process_file(self, input_file: str, output_file: str = "") -> Dict:
        """
        Обработка текстового файла.
        
        Args:
            input_file: Путь к входному файлу
            output_file: Путь к выходному файлу (опционально)
            
        Returns:
            Словарь с результатами обработки
            
        Raises:
            FileNotFoundError: Если входной файл не найден
        """
        if not os.path.exists(input_file):
            raise FileNotFoundError(f"Input file not found: {input_file}")

        print(f"Processing file: {input_file}")

        with open(input_file, "r", encoding="utf-8") as f:
            text = f.read()

        results = self.extract_circumstances(text)

        output_data = {
            "input_file": input_file,
            "total_sentences": len(results),
            "total_entities": sum(len(r["entities"]) for r in results),
            "results": results,
        }

        if output_file:
            self._save_results(output_data, output_file)

        self._print_statistics(output_data)

        return output_data

    def _save_results(self, output_data: Dict, output_file: str) -> None:
        """
        Сохранение результатов в файл.
        
        Args:
            output_data: Данные для сохранения
            output_file: Путь к выходному файлу
        """
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        with open(output_file, "w", encoding="utf-8") as f:
            json.dump(output_data, f, ensure_ascii=False, indent=2)
        print(f"✓ Results saved to: {output_file}")

    @staticmethod
    def _print_statistics(output_data: Dict) -> None:
        """
        Вывод статистики обработки.
        
        Args:
            output_data: Данные с результатами
        """
        print(f"\nProcessed {output_data['total_sentences']} sentences")
        print(f"Found {output_data['total_entities']} entities")
