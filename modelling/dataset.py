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
            padding='max_length',
            truncation=True,
            return_attention_mask=True,
            return_tensors='pt'
        )
        
        item = {
            'input_ids': encoding['input_ids'].flatten(),
            'attention_mask': encoding['attention_mask'].flatten()
        }
        
        if self.labels is not None:
            labels = self.labels[idx]
            # Выравниваем метки с токенами
            label_ids = self.align_labels_with_tokens(text, labels, encoding)
            
            # Паддинг меток
            padded_labels = torch.zeros(self.max_len, dtype=torch.long)
            padded_labels[:len(label_ids)] = torch.tensor(label_ids)
            
            item['labels'] = padded_labels
        
        return item
    
    def align_labels_with_tokens(self, text, original_labels, tokenized):
        """Выравниваем метки с субтокенами BERT"""
        # Получаем word_ids только если токенизатор быстрый
        if hasattr(tokenized, 'word_ids') and callable(tokenized.word_ids):
            word_ids = tokenized.word_ids()
        else:
            # Для медленных токенизаторов используем альтернативный подход
            word_ids = self._get_word_ids_fallback(text, tokenized['input_ids'])
        
        aligned_labels = []
        current_word = None
        
        for word_id in word_ids:
            if word_id is None:
                # Специальные токены
                aligned_labels.append(Config.LABEL2ID['O'])
            elif word_id != current_word:
                # Новое слово
                current_word = word_id
                label = original_labels[word_id] if word_id < len(original_labels) else 'O'
                aligned_labels.append(Config.LABEL2ID.get(label, Config.LABEL2ID['O']))
            else:
                # Продолжение слова (субтокены)
                if word_id < len(original_labels) and original_labels[word_id] != 'O':
                    # Если метка начинается с B-, продолжаем с I-
                    if original_labels[word_id].startswith('B-'):
                        aligned_labels.append(
                            Config.LABEL2ID.get('I-' + original_labels[word_id][2:], Config.LABEL2ID['O'])
                        )
                    else:
                        aligned_labels.append(Config.LABEL2ID.get(original_labels[word_id], Config.LABEL2ID['O']))
                else:
                    aligned_labels.append(Config.LABEL2ID['O'])
        
        return aligned_labels
    
    def _get_word_ids_fallback(self, text, token_ids):
        """Альтернативный метод получения word_ids для медленных токенизаторов"""
        # Конвертируем токены обратно в текст для анализа
        tokens = self.tokenizer.convert_ids_to_tokens(token_ids)
        
        word_ids = []
        word_id = -1
        in_special = False
        
        for i, token in enumerate(tokens):
            if token in [self.tokenizer.cls_token, self.tokenizer.sep_token, self.tokenizer.pad_token]:
                word_ids.append(None)
                in_special = True
            elif token.startswith('##'):
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
    def split_into_sentences(text, language='russian'):
        """Разделение текста на предложения"""
        try:
            nltk.data.find('tokenizers/punkt')
        except LookupError:
            nltk.download('punkt')
        
        try:
            from nltk.tokenize import sent_tokenize
            return sent_tokenize(text, language=language)
        except Exception as e:
            print(f"Error in sentence tokenization: {e}")
            # Простое разделение по точкам как запасной вариант
            sentences = []
            for sent in text.split('.'):
                sent = sent.strip()
                if sent:
                    sentences.append(sent + '.')
            return sentences
    
    @staticmethod
    def load_dataset(filepath):
        """Загрузка размеченного датасета"""
        if not os.path.exists(filepath):
            raise FileNotFoundError(f"Dataset file not found: {filepath}")
        
        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        texts = []
        labels = []
        
        for item in data:
            tokens = item['tokens']
            tags = item['tags']
            
            if len(tokens) != len(tags):
                print(f"Warning: Mismatch in tokens/tags length in item: {item}")
                continue
            
            text = ' '.join(tokens)
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
        
        with open(filepath, 'w', encoding='utf-8') as f:
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
                return_attention_mask=True
            )
            sequences.append({
                'input_ids': encoding['input_ids'],
                'attention_mask': encoding['attention_mask'],
                'text': sentence
            })
        
        return sequences
