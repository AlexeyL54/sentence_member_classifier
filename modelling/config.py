import torch
import json
import os


class Config:
    # Модель
    MODEL_NAME = "DeepPavlov/rubert-base-cased"
    NUM_LABELS = 25
    MAX_LEN = 128
    BATCH_SIZE = 8
    LEARNING_RATE = 2e-5
    EPOCHS = 10
    DROPOUT = 0.1

    # CRF (опционально)
    USE_CRF = True

    # Пути
    BASE_DIR = os.getcwd()
    MODEL_SAVE_PATH = os.path.join(BASE_DIR, "models/bert_ner_model")
    TOKENIZER_SAVE_PATH = os.path.join(BASE_DIR, "models/tokenizer")
    ONNX_SAVE_PATH = os.path.join(BASE_DIR, "models/bert_ner_model.onnx")
    DATASET_PATH = os.path.join(BASE_DIR, "data/dataset.json")
    TRAIN_CONFIG_PATH = os.path.join(BASE_DIR, "data/train_config.json")
    VOCAB_PATH = os.path.join(BASE_DIR, "models/vocab.txt")

    # Классы для NER (BIO-схема)
    LABEL_LIST = [
        "O",
        "B-MANNER",
        "I-MANNER",
        "B-TIME",
        "I-TIME",
        "B-DEGREE",
        "I-DEGREE",
        "B-CONDITION",
        "I-CONDITION",
        "B-CAUSE",
        "I-CAUSE",
        "B-CONCESSION",
        "I-CONCESSION",
        "B-LOCATION",
        "I-LOCATION",
        "B-PURPOSE",
        "I-PURPOSE",
        "B-SUBJECT",
        "I-SUBJECT",
        "B-PREDICATE",
        "I-PREDICATE",
        "B-DEFINITION",
        "I-DEFINITION",
        "B-ADDITION",
        "I-ADDITION",
    ]

    # Проверяем соответствие количества меток
    assert len(LABEL_LIST) == NUM_LABELS, (
        f"LABEL_LIST length ({len(LABEL_LIST)}) != NUM_LABELS ({NUM_LABELS})"
    )

    # Map меток
    LABEL2ID = {label: i for i, label in enumerate(LABEL_LIST)}
    ID2LABEL = {i: label for i, label in enumerate(LABEL_LIST)}

    # Специальные токены
    PAD_TOKEN_ID = 0
    UNK_TOKEN_ID = 100
    CLS_TOKEN_ID = 101
    SEP_TOKEN_ID = 102

    # Устройство
    DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

    @classmethod
    def save_metadata(cls):
        """Сохранение метаданных модели"""
        metadata = {
            "model_type": "bert_ner",
            "model_name": cls.MODEL_NAME,
            "num_labels": cls.NUM_LABELS,
            "max_len": cls.MAX_LEN,
            "labels": cls.LABEL_LIST,
            "id2label": cls.ID2LABEL,
            "label2id": cls.LABEL2ID,
            "pad_token_id": cls.PAD_TOKEN_ID,
            "special_tokens": {
                "pad": "[PAD]",
                "unk": "[UNK]",
                "cls": "[CLS]",
                "sep": "[SEP]",
                "mask": "[MASK]",
            },
            "sentence_parts": {
                "circumstances": [
                    "MANNER",
                    "TIME",
                    "DEGREE",
                    "CONDITION",
                    "CAUSE",
                    "CONCESSION",
                    "LOCATION",
                    "PURPOSE",
                ],
                "main_parts": ["SUBJECT", "PREDICATE"],
                "secondary_parts": [
                    "ADDITION",
                    "DEFINITION",
                ],
            },
        }

        os.makedirs(os.path.dirname(cls.MODEL_SAVE_PATH), exist_ok=True)
        metadata_path = os.path.join(cls.MODEL_SAVE_PATH, "metadata.json")

        with open(metadata_path, "w", encoding="utf-8") as f:
            json.dump(metadata, f, ensure_ascii=False, indent=2)

        print(f"Metadata saved to {metadata_path}")

    @classmethod
    def load_train_config(cls):
        """Загрузка конфигурации обучения из файла"""
        if os.path.exists(cls.TRAIN_CONFIG_PATH):
            with open(cls.TRAIN_CONFIG_PATH, "r", encoding="utf-8") as f:
                print(f"Opening {cls.TRAIN_CONFIG_PATH}")
                config_data = json.load(f)

            # Обновляем параметры из файла
            cls.EPOCHS = config_data.get("epochs", cls.EPOCHS)
            cls.BATCH_SIZE = config_data.get("batch_size", cls.BATCH_SIZE)
            cls.LEARNING_RATE = config_data.get("learning_rate", cls.LEARNING_RATE)
            cls.MAX_LEN = config_data.get("max_len", cls.MAX_LEN)
            cls.MODEL_NAME = config_data.get("model_name", cls.MODEL_NAME)

            print(f"Loaded train config from {cls.TRAIN_CONFIG_PATH}")
        else:
            print("Train config not found, using defaults")
