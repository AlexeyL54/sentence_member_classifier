"""
Модуль для обучения модели распознавания обстоятельств в тексте.

Содержит класс Trainer для управления процессом обучения,
функции для подготовки данных и точку входа в программу.
"""

import torch
import torch.optim as optim
from torch.utils.data import DataLoader
from tqdm import tqdm
import os
import random
from typing import Tuple, List
from transformers import AutoTokenizer, AutoModelForTokenClassification

from config import Config
from dataset import CircumstanceDataset, DataProcessor


class Trainer:
    """
    Класс для обучения и оценки модели токенизации.

    Предоставляет методы для обучения одной эпохи, оценки на валидационной выборке,
    полного цикла обучения, сохранения модели и экспорта в ONNX формат.
    """

    def __init__(self, model, device=Config.DEVICE):
        """
        Инициализация тренера.

        Args:
            model: Модель для обучения
            device: Устройство для вычислений (cpu/cuda)
        """
        self.model = model.to(device)
        self.device = device

    def train_epoch(self, dataloader, optimizer, scheduler):
        """
        Обучение модели на одной эпохе.

        Args:
            dataloader: DataLoader с тренировочными данными
            optimizer: Оптимизатор
            scheduler: Планировщик learning rate

        Returns:
            float: Среднее значение функции потерь за эпоху
        """
        self.model.train()
        total_loss = 0

        for batch in tqdm(dataloader, desc="Training"):
            optimizer.zero_grad()

            input_ids = batch["input_ids"].to(self.device)
            attention_mask = batch["attention_mask"].to(self.device)
            labels = batch["labels"].to(self.device)

            outputs = self.model(
                input_ids=input_ids, attention_mask=attention_mask, labels=labels
            )

            loss = outputs.loss
            loss.backward()

            torch.nn.utils.clip_grad_norm_(self.model.parameters(), 1.0)
            optimizer.step()

            if scheduler:
                scheduler.step()

            total_loss += loss.item()

        return total_loss / len(dataloader)

    def evaluate(self, dataloader) -> Tuple[float, List[int], List[int]]:
        """
        Оценка модели на валидационной выборке.

        Args:
            dataloader: DataLoader с валидационными данными

        Returns:
            Tuple[float, List[int], List[int]]:
                - Среднее значение функции потерь
                - Список предсказанных меток
                - Список истинных меток
        """
        self.model.eval()
        total_loss = 0
        predictions = []
        true_labels = []

        with torch.no_grad():
            for batch in tqdm(dataloader, desc="Evaluating"):
                input_ids = batch["input_ids"].to(self.device)
                attention_mask = batch["attention_mask"].to(self.device)
                labels = batch["labels"].to(self.device)

                outputs = self.model(
                    input_ids=input_ids, attention_mask=attention_mask, labels=labels
                )

                loss = outputs.loss
                logits = outputs.logits

                if loss is not None:
                    total_loss += loss.item()

                # Преобразуем logits в предсказания
                preds = torch.argmax(logits, dim=-1)

                # Убираем паддинг
                for i in range(len(preds)):
                    mask = attention_mask[i].bool()
                    pred = preds[i][mask]
                    label = labels[i][mask]

                    predictions.extend(pred.cpu().numpy())
                    true_labels.extend(label.cpu().numpy())

        avg_loss = total_loss / len(dataloader) if total_loss > 0 else 0
        return avg_loss, predictions, true_labels

    def train(self, train_dataloader, val_dataloader, epochs=Config.EPOCHS):
        """
        Полный цикл обучения модели.

        Args:
            train_dataloader: DataLoader с тренировочными данными
            val_dataloader: DataLoader с валидационными данными
            epochs: Количество эпох обучения
        """
        # Оптимизатор
        optimizer = optim.AdamW(self.model.parameters(), lr=Config.LEARNING_RATE)

        # Scheduler
        total_steps = len(train_dataloader) * epochs
        scheduler = optim.lr_scheduler.LinearLR(
            optimizer, start_factor=1.0, end_factor=0.1, total_iters=total_steps
        )

        best_loss = float("inf")

        for epoch in range(epochs):
            print(f"\nEpoch {epoch + 1}/{epochs}")
            print("-" * 50)

            # Обучение
            train_loss = self.train_epoch(train_dataloader, optimizer, scheduler)
            print(f"Train Loss: {train_loss:.4f}")

            # Валидация
            val_loss, _, _ = self.evaluate(val_dataloader)
            print(f"Val Loss: {val_loss:.4f}")

            # Сохранение лучшей модели
            if val_loss < best_loss:
                best_loss = val_loss
                self.save_model()
                print(f"✓ Best model saved! Loss: {val_loss:.4f}")

    def save_model(self, path=Config.MODEL_SAVE_PATH):
        """
        Сохранение модели и токенизатора.

        Args:
            path: Путь для сохранения модели
        """
        os.makedirs(path, exist_ok=True)

        # Сохраняем модель
        self.model.save_pretrained(path)

        # Сохраняем токенизатор
        tokenizer_path = os.path.join(path, "tokenizer")
        tokenizer = AutoTokenizer.from_pretrained(Config.MODEL_NAME)
        tokenizer.save_pretrained(tokenizer_path)

        # Сохраняем метаданные
        Config.save_metadata()

        print(f"✓ Model saved to {path}")
        print(f"✓ Tokenizer saved to {tokenizer_path}")

    def export_to_onnx(self, onnx_path=Config.ONNX_SAVE_PATH):
        """
        Экспорт модели в ONNX формат.

        Args:
            onnx_path: Путь для сохранения ONNX модели
        """
        self.model.eval()

        # Пример входных данных
        dummy_input_ids = torch.randint(0, 30000, (1, Config.MAX_LEN)).to(self.device)
        dummy_attention_mask = torch.ones((1, Config.MAX_LEN)).to(self.device)

        torch.onnx.export(
            self.model,
            (dummy_input_ids, dummy_attention_mask),
            onnx_path,
            export_params=True,
            opset_version=14,
            do_constant_folding=True,
            input_names=["input_ids", "attention_mask"],
            output_names=["logits"],
            dynamic_axes={
                "input_ids": {0: "batch_size", 1: "sequence_length"},
                "attention_mask": {0: "batch_size", 1: "sequence_length"},
                "logits": {0: "batch_size", 1: "sequence_length"},
            },
            verbose=False,
        )
        print(f"✓ ONNX model exported to {onnx_path}")


def train_val_split(
    texts: List[str], labels: List[List[str]], val_size: float = 0.2, seed: int = 42
) -> Tuple[List[str], List[str], List[List[str]], List[List[str]]]:
    """
    Разделение данных на обучающую и валидационную выборки.

    Args:
        texts: Список текстов
        labels: Список последовательностей меток
        val_size: Доля валидационной выборки (от 0 до 1)
        seed: Seed для воспроизводимости

    Returns:
        Tuple:
            - Обучающие тексты
            - Валидационные тексты
            - Обучающие метки
            - Валидационные метки
    """
    n_samples = len(texts)
    indices = list(range(n_samples))

    random.seed(seed)
    random.shuffle(indices)

    split_idx = int(n_samples * (1 - val_size))

    train_indices = indices[:split_idx]
    val_indices = indices[split_idx:]

    train_texts = [texts[i] for i in train_indices]
    train_labels = [labels[i] for i in train_indices]

    val_texts = [texts[i] for i in val_indices]
    val_labels = [labels[i] for i in val_indices]

    return train_texts, val_texts, train_labels, val_labels


def load_and_prepare_data(dataset_path: str) -> Tuple[List[str], List[List[str]]]:
    """
    Загрузка и подготовка данных для обучения.

    Args:
        dataset_path: Путь к файлу с датасетом

    Returns:
        Tuple[List[str], List[List[str]]]: Тексты и соответствующие метки

    Raises:
        Exception: Если не удалось загрузить данные
    """
    print(f"\nLoading dataset from {dataset_path}")
    try:
        texts, labels = DataProcessor.load_dataset(dataset_path)
        return texts, labels
    except Exception as e:
        print(f"Error loading dataset: {e}")
        raise


def display_training_configuration() -> None:
    """Вывод конфигурации обучения в консоль."""
    print("=" * 60)
    print("TRAINING CONFIGURATION")
    print("=" * 60)
    print(f"Model: {Config.MODEL_NAME}")
    print(f"Epochs: {Config.EPOCHS}")
    print(f"Batch size: {Config.BATCH_SIZE}")
    print(f"Learning rate: {Config.LEARNING_RATE}")
    print(f"Max sequence length: {Config.MAX_LEN}")
    print(f"Device: {Config.DEVICE}")
    print(f"Use CRF: {Config.USE_CRF}")
    print("=" * 60)


def create_datasets_and_loaders(
    train_texts: List[str],
    train_labels: List[List[str]],
    val_texts: List[str],
    val_labels: List[List[str]],
    tokenizer,
) -> Tuple[DataLoader, DataLoader]:
    """
    Создание датасетов и DataLoader'ов для обучения и валидации.

    Args:
        train_texts: Обучающие тексты
        train_labels: Обучающие метки
        val_texts: Валидационные тексты
        val_labels: Валидационные метки
        tokenizer: Токенизатор

    Returns:
        Tuple[DataLoader, DataLoader]: Тренировочный и валидационный DataLoader
    """
    print("Creating datasets...")
    train_dataset = CircumstanceDataset(train_texts, train_labels, tokenizer)
    val_dataset = CircumstanceDataset(val_texts, val_labels, tokenizer)

    train_dataloader = DataLoader(
        train_dataset, batch_size=Config.BATCH_SIZE, shuffle=True
    )
    val_dataloader = DataLoader(val_dataset, batch_size=Config.BATCH_SIZE)

    return train_dataloader, val_dataloader


def initialize_model() -> AutoModelForTokenClassification:
    """
    Инициализация модели для токенизации.

    Returns:
        AutoModelForTokenClassification: Инициализированная модель
    """
    print("Initializing model...")
    return AutoModelForTokenClassification.from_pretrained(
        Config.MODEL_NAME,
        num_labels=Config.NUM_LABELS,
        id2label=Config.ID2LABEL,
        label2id=Config.LABEL2ID,
    )


def print_training_summary() -> None:
    """Вывод итогов обучения в консоль."""
    print("\n" + "=" * 60)
    print("TRAINING COMPLETED")
    print("=" * 60)
    print(f"Model saved to: {Config.MODEL_SAVE_PATH}")
    print(f"Tokenizer saved to: {Config.MODEL_SAVE_PATH}/tokenizer")
    print(f"ONNX model saved to: {Config.ONNX_SAVE_PATH}")
    print(f"Vocabulary: {Config.VOCAB_PATH}")


def main() -> None:
    """
    Основная функция для запуска процесса обучения.

    Выполняет следующие шаги:
    1. Загружает конфигурацию
    2. Загружает и подготавливает данные
    3. Разделяет данные на train/val
    4. Создает датасеты и DataLoader'ы
    5. Инициализирует модель
    6. Запускает обучение
    7. Экспортирует модель в ONNX
    """
    # Загрузка конфигурации
    # Config.load_train_config()
    display_training_configuration()

    # Загрузка данных
    try:
        texts, labels = load_and_prepare_data(Config.DATASET_PATH)
    except Exception:
        return

    # Разделение на train/val
    print("Splitting data into train/validation sets...")
    train_texts, val_texts, train_labels, val_labels = train_val_split(
        texts, labels, val_size=0.2
    )

    print(f"Train samples: {len(train_texts)}")
    print(f"Validation samples: {len(val_texts)}")

    # Токенизатор
    tokenizer = AutoTokenizer.from_pretrained(Config.MODEL_NAME)

    # Создание датасетов и DataLoader'ов
    train_dataloader, val_dataloader = create_datasets_and_loaders(
        train_texts, train_labels, val_texts, val_labels, tokenizer
    )

    # Модель
    model = initialize_model()

    # Обучение
    print("\n" + "=" * 60)
    print("STARTING TRAINING")
    print("=" * 60)
    trainer = Trainer(model)
    trainer.train(train_dataloader, val_dataloader)

    # Экспорт в ONNX
    print("\nExporting model to ONNX format...")
    trainer.export_to_onnx()

    # Итоги
    print_training_summary()


if __name__ == "__main__":
    main()
