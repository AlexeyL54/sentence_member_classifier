import torch
import json
import os


class Config:
    """
    Класс конфигурации для модели NER (Named Entity Recognition) на основе BERT.

    Содержит все параметры модели, пути к файлам, настройки обучения,
    а также методы для сохранения метаданных и загрузки конфигурации обучения.

    Attributes:
        MODEL_NAME (str): Название предобученной модели из Hugging Face
        NUM_LABELS (int): Количество меток для классификации
        MAX_LEN (int): Максимальная длина последовательности токенов
        BATCH_SIZE (int): Размер батча при обучении
        LEARNING_RATE (float): Скорость обучения
        EPOCHS (int): Количество эпох обучения
        DROPOUT (float): Вероятность dropout для регуляризации
        USE_CRF (bool): Флаг использования CRF слоя
        BASE_DIR (str): Базовая директория проекта
        MODEL_SAVE_PATH (str): Путь для сохранения модели
        TOKENIZER_SAVE_PATH (str): Путь для сохранения токенизатора
        ONNX_SAVE_PATH (str): Путь для сохранения модели в формате ONNX
        DATASET_PATH (str): Путь к файлу с датасетом
        TRAIN_CONFIG_PATH (str): Путь к файлу конфигурации обучения
        VOCAB_PATH (str): Путь к файлу словаря
        LABEL_LIST (list): Список всех меток в BIO-схеме
        LABEL2ID (dict): Словарь соответствия метка -> ID
        ID2LABEL (dict): Словарь соответствия ID -> метка
        PAD_TOKEN_ID (int): ID токена padding
        UNK_TOKEN_ID (int): ID токена unknown
        CLS_TOKEN_ID (int): ID токена [CLS]
        SEP_TOKEN_ID (int): ID токена [SEP]
        DEVICE (torch.device): Устройство для вычислений (CPU/GPU)
    """

    # Модель
    MODEL_NAME = "ai-forever/rubert-base"
    NUM_LABELS = 11
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

    LABEL_LIST = [
        "O",
        "B-ADVERBIAL",  # Все обстоятельства (время, место, причина и т.д.)
        "I-ADVERBIAL",
        "B-SUBJECT",  # Подлежащее
        "I-SUBJECT",
        "B-PREDICATE",  # Сказуемое
        "I-PREDICATE",
        "B-DEFINITION",  # Определение
        "I-DEFINITION",
        "B-ADDITION",  # Дополнение
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
        """
        Сохраняет метаданные модели в JSON файл.

        Создает словарь с метаданными модели, включая тип модели, название,
        количество меток, максимальную длину последовательности, списки меток,
        информацию о специальных токенах и частях предложения.

        Метаданные сохраняются в файл metadata.json в директории модели.

        Returns:
            None

        Raises:
            OSError: Если не удается создать директорию или записать файл
        """
        sentence_parts = {
            "ADVERBIAL": "обстоятельство",
            "SUBJECT": "подлежащее",
            "PREDICATE": "сказуемое",
            "DEFINITION": "определение",
            "ADDITION": "дополнение",
        }

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
            "sentence_parts": sentence_parts,
        }

        os.makedirs(os.path.dirname(cls.MODEL_SAVE_PATH), exist_ok=True)
        metadata_path = os.path.join(cls.MODEL_SAVE_PATH, "metadata.json")

        with open(metadata_path, "w", encoding="utf-8") as f:
            json.dump(metadata, f, ensure_ascii=False, indent=2)

        print(f"Metadata saved to {metadata_path}")


"""    @classmethod
    def load_train_config(cls):
        Загружает конфигурацию обучения из JSON файла.

        Читает файл train_config.json и обновляет параметры обучения:
        количество эпох, размер батча, скорость обучения, максимальную длину
        последовательности и название модели.

        Если файл конфигурации не найден, используются значения по умолчанию.

        Returns:
            None

        Note:
            Файл конфигурации должен иметь следующую структуру:
            {
                "epochs": 10,
                "batch_size": 8,
                "learning_rate": 2e-5,
                "max_len": 128,
                "model_name": "ai-forever/rubert-base"
            }
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
            print("Train config not found, using defaults") """
