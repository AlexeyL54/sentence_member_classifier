"""
Модуль для оценки качества модели распознавания обстоятельств.

Содержит функции для вычисления метрик precision, recall, F1-score
и визуализации результатов.
"""

import numpy as np
import json
import os
from datetime import datetime
from typing import Dict, List, Tuple, Optional, Any
from sklearn.metrics import (
    precision_recall_fscore_support,
    classification_report,
    confusion_matrix,
)
import matplotlib.pyplot as plt

from config import Config
from dataset import DataProcessor
from inference import CircumstancePredictor


class MetricsCalculator:
    """Класс для вычисления метрик качества модели."""

    def __init__(self):
        """
        Инициализация калькулятора метрик.

        Устанавливает списки меток, отображения между ID и метками,
        а также базовые классы для оценки качества распознавания членов предложения.
        """
        self.label_list = Config.LABEL_LIST
        self.id2label = Config.ID2LABEL
        self.label2id = Config.LABEL2ID

        self.base_classes = [
            "O",
            "ADVERBIAL",
            "SUBJECT",
            "PREDICATE",
            "DEFINITION",
            "ADDITION",
        ]
        self.class_names = {
            "O": "не является членом предложения",
            "ADVERBIAL": "обстоятельство",
            "SUBJECT": "подлежащее",
            "PREDICATE": "сказуемое",
            "DEFINITION": "определение",
            "ADDITION": "дополнение",
        }

        self.class_to_id = {cls: i for i, cls in enumerate(self.base_classes)}

    def _get_base_class(self, label: str) -> str:
        """
        Получение базового класса из BIO-метки.

        Args:
            label (str): BIO-метка (например, "B-ADVERBIAL", "I-ADVERBIAL" или "O")

        Returns:
            str: Базовый класс без BIO-префикса
        """
        if label == "O":
            return "O"
        if "-" in label:
            return label.split("-")[1]
        return label

    def convert_to_base_ids(
        self, predictions: List[int], true_labels: List[int]
    ) -> Tuple[List[int], List[int]]:
        """
        Преобразование BIO-меток в ID базовых классов.

        Args:
            predictions (List[int]): Список ID предсказанных BIO-меток
            true_labels (List[int]): Список ID истинных BIO-меток

        Returns:
            Tuple[List[int], List[int]]: Кортеж из двух списков:
                - Список ID базовых классов для предсказаний
                - Список ID базовых классов для истинных меток
        """
        pred_base = []
        true_base = []

        for pred, true in zip(predictions, true_labels):
            pred_label = self.id2label[pred]
            true_label = self.id2label[true]

            pred_base_class = self._get_base_class(pred_label)
            true_base_class = self._get_base_class(true_label)

            pred_base.append(self.class_to_id[pred_base_class])
            true_base.append(self.class_to_id[true_base_class])

        return pred_base, true_base

    def calculate_metrics(
        self, predictions: List[int], true_labels: List[int]
    ) -> Dict[str, Any]:
        """
        Расчет precision, recall, f1 с weighted average.

        Args:
            predictions (List[int]): Список ID предсказанных BIO-меток
            true_labels (List[int]): Список ID истинных BIO-меток

        Returns:
            Dict[str, Any]: Словарь с метриками, содержащий:
                - precision (float): Взвешенная precision
                - recall (float): Взвешенная recall
                - f1 (float): Взвешенный F1-score
                - support (int): Общее количество токенов
                - per_class (Dict): Метрики для каждого класса
        """
        pred_base, true_base = self.convert_to_base_ids(predictions, true_labels)
        labels = list(range(len(self.base_classes)))

        # Получаем per-class метрики
        per_class = self._get_per_class_metrics(true_base, pred_base)

        # Вычисляем weighted average
        result = precision_recall_fscore_support(
            true_base, pred_base, average="weighted", labels=labels, zero_division=0
        )

        # weighted average возвращает (precision, recall, f1, None)
        if result is None:
            return {
                "precision": 0.0,
                "recall": 0.0,
                "f1": 0.0,
                "support": sum([v["support"] for v in per_class.values()]),
                "per_class": per_class,
            }

        precision, recall, f1, _ = result  # Игнорируем четвертый элемент (None)

        total_support = sum([v["support"] for v in per_class.values()])

        return {
            "precision": float(precision) if precision is not None else 0.0,
            "recall": float(recall) if recall is not None else 0.0,
            "f1": float(f1) if f1 is not None else 0.0,
            "support": total_support,
            "per_class": per_class,
        }

    def _get_per_class_metrics(
        self, true_labels: List[int], predictions: List[int]
    ) -> Dict[str, Dict[str, float]]:
        """
        Расчет метрик для каждого класса.

        Args:
            true_labels (List[int]): Список ID истинных меток базовых классов
            predictions (List[int]): Список ID предсказанных меток базовых классов

        Returns:
            Dict[str, Dict[str, float]]: Словарь, где ключ - название класса,
                значение - словарь с метриками precision, recall, f1, support
        """

        # Get all possible labels (0 to len(base_classes)-1)
        # all_labels = list(range(len(self.base_classes)))
        actual_classes = sorted(set(true_labels))

        # Calculate metrics for all classes
        result = precision_recall_fscore_support(
            true_labels, predictions, labels=actual_classes, zero_division=0
        )

        if result is None:
            return {
                self.class_names[cls]: {
                    "precision": 0.0,
                    "recall": 0.0,
                    "f1": 0.0,
                    "support": 0,
                }
                for cls in self.base_classes
            }

        precision, recall, f1, support = result

        per_class = {}
        for i, class_name in enumerate(actual_classes):
            display_name = self.base_classes[class_name]
            per_class[display_name] = {
                "precision": float(precision[i]) if precision is not None else 0.0,
                "recall": float(recall[i]) if recall is not None else 0.0,
                "f1": float(f1[i]) if f1 is not None else 0.0,
                "support": int(support[i]) if support is not None else 0,
            }

        return per_class

    def generate_report(
        self,
        predictions: List[int],
        true_labels: List[int],
        output_file: Optional[str] = None,
    ) -> str:
        """
        Генерация отчета о классификации.

        Args:
            predictions (List[int]): Список ID предсказанных BIO-меток
            true_labels (List[int]): Список ID истинных BIO-меток
            output_file (Optional[str]): Путь для сохранения отчета.
                Если указан, отчет сохраняется в файл

        Returns:
            str: Текст отчета о классификации
        """
        pred_base, true_base = self.convert_to_base_ids(predictions, true_labels)

        unique_classes = sorted(set(true_base))
        target_names = [self.class_names[self.base_classes[i]] for i in unique_classes]

        labels = unique_classes

        report = classification_report(
            true_base,
            pred_base,
            target_names=target_names,
            labels=labels,
            zero_division=0,
        )

        full_report = self._format_report_header(len(pred_base)) + report

        if output_file:
            self._save_report(full_report, output_file)

        return full_report

    def _format_report_header(self, total_tokens: int) -> str:
        """
        Форматирование заголовка отчета.

        Args:
            total_tokens (int): Общее количество токенов

        Returns:
            str: Отформатированный заголовок
        """
        header = f"\n{'=' * 70}\n"
        header += f"CLASSIFICATION REPORT\n"
        header += f"{'=' * 70}\n"
        header += f"Total tokens evaluated: {total_tokens}\n"
        header += f"{'=' * 70}\n\n"
        return header

    def _save_report(self, report: str, output_file: str) -> None:
        """
        Сохранение отчета в файл.

        Args:
            report (str): Текст отчета
            output_file (str): Путь для сохранения файла
        """
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"Report saved to {output_file}")

    def calculate_confusion_matrix(
        self, predictions: List[int], true_labels: List[int]
    ) -> np.ndarray:
        """
        Расчет матрицы ошибок.

        Args:
            predictions (List[int]): Список ID предсказанных BIO-меток
            true_labels (List[int]): Список ID истинных BIO-меток

        Returns:
            np.ndarray: Матрица ошибок размером (n_classes, n_classes),
                где строки - истинные классы, столбцы - предсказанные
        """
        pred_base, true_base = self.convert_to_base_ids(predictions, true_labels)
        return confusion_matrix(
            true_base, pred_base, labels=list(range(len(self.base_classes)))
        )

    def plot_confusion_matrix(
        self, cm: np.ndarray, output_path: Optional[str] = None
    ) -> None:
        """
        Визуализация матрицы ошибок.

        Args:
            cm (np.ndarray): Матрица ошибок для визуализации
            output_path (Optional[str]): Путь для сохранения графика.
                Если указан, график сохраняется в файл
        """
        if cm.size == 0:
            print("Cannot plot empty confusion matrix")
            return

        target_names = [self.class_names[c] for c in self.base_classes]

        # Нормализация по строкам с коррекцией погрешностей
        row_sums = cm.sum(axis=1, keepdims=True)
        cm_norm = np.zeros_like(cm, dtype=float)
        mask = row_sums > 0
        cm_norm[mask.flatten()] = cm[mask.flatten()] / row_sums[mask.flatten()]

        # Корректируем суммы строк, чтобы они были точно равны 1
        for i in range(cm_norm.shape[0]):
            row_sum = cm_norm[i, :].sum()
            if row_sum > 0 and abs(row_sum - 1.0) > 1e-10:
                # Нормализуем строку заново с большей точностью
                if cm[i, :].sum() > 0:
                    cm_norm[i, :] = cm[i, :] / cm[i, :].sum()

        self._create_confusion_matrix_plot(cm_norm, target_names, output_path)

    def _create_confusion_matrix_plot(
        self, cm: np.ndarray, target_names: List[str], output_path: Optional[str]
    ) -> None:
        """
        Создание графика матрицы ошибок.

        Args:
            cm (np.ndarray): Нормализованная матрица ошибок
            target_names (List[str]): Названия классов
            output_path (Optional[str]): Путь для сохранения графика
        """
        plt.figure(figsize=(12, 10))
        plt.imshow(cm, interpolation="nearest", cmap=plt.cm.Blues, vmin=0, vmax=1)
        plt.title("Normalized Confusion Matrix (by true labels)", fontsize=14)
        plt.colorbar()

        plt.xticks(
            np.arange(len(target_names)),
            target_names,
            rotation=45,
            ha="right",
            fontsize=10,
        )
        plt.yticks(np.arange(len(target_names)), target_names, fontsize=10)

        self._add_values_to_cells(cm)

        plt.tight_layout()
        plt.ylabel("True label", fontsize=12)
        plt.xlabel("Predicted label", fontsize=12)

        if output_path:
            self._save_plot(output_path)

        plt.close()

    def _add_values_to_cells(self, cm: np.ndarray) -> None:
        """
        Добавление значений в ячейки матрицы с корректным округлением.

        Args:
            cm (np.ndarray): Матрица ошибок для отображения значений
        """
        if cm.size == 0:
            return

        # Для каждой строки проверяем и корректируем округление
        for i in range(cm.shape[0]):
            row_sum = cm[i, :].sum()
            if row_sum > 0 and abs(row_sum - 1.0) > 1e-6:
                # Если сумма строки не равна 1 из-за погрешности, нормализуем заново
                cm[i, :] = cm[i, :] / row_sum

        thresh = cm.max() / 2.0 if cm.max() > 0 else 0.5

        for i in range(cm.shape[0]):
            for j in range(cm.shape[1]):
                if cm[i, j] > 0:
                    value = cm[i, j]
                    # Округляем до двух знаков для отображения
                    if value >= 0.995:  # Очень близко к 1
                        text = "1.00"
                    elif value <= 0.005:  # Очень близко к 0
                        continue  # Не показываем очень маленькие значения
                    else:
                        text = f"{value:.2f}"

                    plt.text(
                        j,
                        i,
                        text,
                        ha="center",
                        va="center",
                        color="white" if value > thresh else "black",
                        fontsize=8,
                    )

    def _save_plot(self, output_path: str) -> None:
        """
        Сохранение графика.

        Args:
            output_path (str): Путь для сохранения графика
        """
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        plt.savefig(output_path, dpi=300, bbox_inches="tight")
        print(f"Confusion matrix saved to {output_path}")


def collect_predictions(
    predictor: CircumstancePredictor,
    texts: List[str],
    labels: List[List[str]],
) -> Tuple[List[int], List[int]]:
    """
    Сбор предсказаний модели для всех текстов.

    Args:
        predictor (CircumstancePredictor): Объект предсказателя модели
        texts (List[str]): Список текстов для обработки
        labels (List[List[str]]): Список списков истинных меток для каждого текста

    Returns:
        Tuple[List[int], List[int]]: Кортеж из двух списков:
            - Список ID предсказанных меток для всех токенов
            - Список ID истинных меток для всех токенов
    """
    all_predictions = []
    all_true_labels = []

    for text, true_seq in zip(texts, labels):
        tokens, offsets = predictor.predict_sentence(text)
        pred_ids = _get_prediction_ids(tokens)

        # Извлекаем смещения из объектов TokenInfo
        token_offsets = [(o[0], o[1]) for o in offsets]
        true_ids = _align_true_labels(tokens, true_seq, token_offsets, text)

        pred_ids, true_ids = _align_lengths(pred_ids, true_ids)

        all_predictions.extend(pred_ids)
        all_true_labels.extend(true_ids)

    return all_predictions, all_true_labels


def _get_prediction_ids(tokens: List) -> List[int]:
    """
    Получение ID предсказанных меток.

    Args:
        tokens (List): Список токенов с атрибутами label

    Returns:
        List[int]: Список ID предсказанных меток
    """
    pred_ids = []
    for token in tokens:
        label_id = Config.LABEL2ID.get(token.label, Config.LABEL2ID["O"])
        pred_ids.append(label_id)
    return pred_ids


def _align_true_labels(tokens: List, true_seq: List[str], offsets: List[Tuple[int, int]], original_texts: str) -> List[int]:
    """
    Выравнивание истинных меток с токенами на основе символьных смещений.

    Args:
        tokens (List): Список токенов из модели
        true_seq (List[str]): Список истинных меток для оригинальных токенов
        offsets (List[Tuple[int, int]]): Смещения токенов модели в тексте
        original_texts (str): Исходный текст для вычисления позиций

    Returns:
        List[int]: Список ID истинных меток, выровненных по длине токенов
    """
    # Сначала восстанавливаем позиции оригинальных токенов в тексте
    # Это нужно потому, что true_seq соответствует оригинальным токенам,
    # а не субтокенам BERT

    # Разбираем текст на оригинальные токены с их позициями
    # Предполагаем, что true_seq соответствует токенам, разделенным пробелами
    words = original_texts.split()

    if len(words) != len(true_seq):
        # Если количество слов не совпадает с количеством меток,
        # используем fallback стратегию
        true_ids = []
        for i in range(len(tokens)):
            if i < len(true_seq):
                true_ids.append(Config.LABEL2ID.get(true_seq[i], Config.LABEL2ID["O"]))
            else:
                true_ids.append(Config.LABEL2ID["O"])
        return true_ids

    # Строим маппинг от позиции в тексте к метке
    word_offsets = []
    pos = 0
    for i, word in enumerate(words):
        start = original_texts.find(word, pos)
        if start == -1:
            # Слово не найдено, используем текущую позицию
            start = pos
        end = start + len(word)
        word_offsets.append((start, end, true_seq[i]))
        pos = end

    # Для каждого токена модели находим соответствующую метку
    true_ids = []
    for token, offset in zip(tokens, offsets):
        char_pos = offset[0]
        assigned_label = "O"

        # Находим, какому оригинальному токену принадлежит этот субтокен
        for orig_start, orig_end, orig_label in word_offsets:
            if orig_start <= char_pos < orig_end:
                assigned_label = orig_label
                break

        true_ids.append(Config.LABEL2ID.get(assigned_label, Config.LABEL2ID["O"]))

    return true_ids


def _align_lengths(
    pred_ids: List[int], true_ids: List[int]
) -> Tuple[List[int], List[int]]:
    """
    Выравнивание длин предсказаний и истинных меток.

    Args:
        pred_ids (List[int]): Список ID предсказанных меток
        true_ids (List[int]): Список ID истинных меток

    Returns:
        Tuple[List[int], List[int]]: Кортеж из двух списков одинаковой длины:
            - Список предсказанных меток
            - Список истинных меток
    """
    if len(true_ids) < len(pred_ids):
        true_ids.extend([Config.LABEL2ID["O"]] * (len(pred_ids) - len(true_ids)))
    else:
        true_ids = true_ids[: len(pred_ids)]
        pred_ids = pred_ids[: len(true_ids)]
    return pred_ids, true_ids


def print_metrics_summary(metrics: Dict[str, Any]) -> None:
    """
    Вывод сводки метрик в консоль.

    Args:
        metrics (Dict[str, Any]): Словарь с метриками из calculate_metrics()
    """
    _print_aggregated_metrics(metrics)
    _print_per_class_metrics(metrics)
    _print_class_comparison(metrics)


def _print_aggregated_metrics(metrics: Dict[str, Any]) -> None:
    """
    Вывод агрегированных метрик.

    Args:
        metrics (Dict[str, Any]): Словарь с метриками
    """
    print("\n" + "=" * 70)
    print("AGGREGATED METRICS (Weighted Average)")
    print("=" * 70)
    print(f"Precision: {metrics['precision']:.4f}")
    print(f"Recall: {metrics['recall']:.4f}")
    print(f"F1-score: {metrics['f1']:.4f}")
    print(f"Total support: {metrics['support']}")


def _print_per_class_metrics(metrics: Dict[str, Any]) -> None:
    """
    Вывод метрик для каждого класса.

    Args:
        metrics (Dict[str, Any]): Словарь с метриками, содержащий per_class
    """
    print("\n" + "=" * 70)
    print("PER-CLASS METRICS")
    print("=" * 70)
    print(
        f"\n{'Class':<30} {'Precision':>10} {'Recall':>10} {'F1-score':>10} {'Support':>10}"
    )
    print("-" * 70)

    # metrics["per_class"] уже содержит русские названия классов
    # сортируем по убыванию F1
    sorted_classes = sorted(
        metrics["per_class"].items(), key=lambda x: x[1]["f1"], reverse=True
    )

    for class_name, class_metrics in sorted_classes:
        print(
            f"{class_name:<30} "
            f"{class_metrics['precision']:>10.4f} "
            f"{class_metrics['recall']:>10.4f} "
            f"{class_metrics['f1']:>10.4f} "
            f"{class_metrics['support']:>10}"
        )


def _print_class_comparison(metrics: Dict[str, Any]) -> None:
    """
    Вывод сравнения между членами предложения.

    Args:
        metrics (Dict[str, Any]): Словарь с метриками, содержащий per_class
    """
    print("\n" + "=" * 70)
    print("COMPARISON BETWEEN SENTENCE PARTS")
    print("=" * 70)

    # Исключаем "не является членом предложения"
    sentence_parts = {
        k: v
        for k, v in metrics["per_class"].items()
        if k != "не является членом предложения"
    }

    if not sentence_parts:
        print("No sentence parts found in metrics")
        return

    best_class = max(sentence_parts.items(), key=lambda x: x[1]["f1"])
    worst_class = min(sentence_parts.items(), key=lambda x: x[1]["f1"])

    print(f"\nBest performing: {best_class[0]}")
    print(f"  F1-score: {best_class[1]['f1']:.4f}")
    print(f"  Support: {best_class[1]['support']}")

    print(f"\nWorst performing: {worst_class[0]}")
    print(f"  F1-score: {worst_class[1]['f1']:.4f}")
    print(f"  Support: {worst_class[1]['support']}")

    avg_f1 = np.mean([v["f1"] for v in sentence_parts.values()])
    avg_precision = np.mean([v["precision"] for v in sentence_parts.values()])
    avg_recall = np.mean([v["recall"] for v in sentence_parts.values()])

    print(f"\nAverage across sentence parts (macro):")
    print(f"  Precision: {avg_precision:.4f}")
    print(f"  Recall: {avg_recall:.4f}")
    print(f"  F1-score: {avg_f1:.4f}")

    f1_diff = best_class[1]["f1"] - worst_class[1]["f1"]
    print(f"\nPerformance gap (best - worst): {f1_diff:.4f}")


def save_results(
    metrics: Dict[str, Any],
    report: str,
    output_dir: str,
) -> None:
    """
    Сохранение результатов оценки в файлы.

    Args:
        metrics (Dict[str, Any]): Словарь с метриками
        report (str): Текст отчета о классификации
        output_dir (str): Директория для сохранения результатов
    """
    os.makedirs(output_dir, exist_ok=True)

    results = _prepare_results_json(metrics, report)

    json_path = os.path.join(output_dir, "evaluation_results.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=2)
    print(f"\nResults saved to {json_path}")


def _prepare_results_json(metrics: Dict[str, Any], report: str) -> Dict[str, Any]:
    """
    Подготовка данных для JSON.

    Args:
        metrics (Dict[str, Any]): Словарь с метриками
        report (str): Текст отчета о классификации

    Returns:
        Dict[str, Any]: Структурированные данные для сохранения в JSON
    """
    timestamp = datetime.now().isoformat()
    date = datetime.now().strftime("%Y-%m-%d")
    time = datetime.now().strftime("%H:%M:%S")

    # Вычисляем среднее по членам предложения (исключая O)
    sentence_parts_avg_f1 = np.mean(
        [
            v["f1"]
            for k, v in metrics["per_class"].items()
            if k != "не является членом предложения"
        ]
    )

    return {
        "evaluation_info": {
            "timestamp": timestamp,
            "date": date,
            "time": time,
            "model_name": Config.MODEL_NAME,
            "model_path": Config.MODEL_SAVE_PATH,
        },
        "weighted_metrics": {
            "precision": metrics["precision"],
            "recall": metrics["recall"],
            "f1": metrics["f1"],
            "support": metrics["support"],
        },
        "per_class_metrics": metrics["per_class"],
        "comparison": {
            "sentence_parts_avg_f1": float(sentence_parts_avg_f1),
        },
        "classification_report": report,
    }


def print_dataset_stats(texts: List[str], labels: List[List[str]]) -> None:
    """
    Вывод статистики по датасету.

    Args:
        texts (List[str]): Список текстов
        labels (List[List[str]]): Список списков меток для каждого текста
    """
    all_labels_flat = [label for seq in labels for label in seq]
    label_counts = {}

    for label in all_labels_flat:
        base_class = label.split("-")[1] if "-" in label else label
        label_counts[base_class] = label_counts.get(base_class, 0) + 1

    class_names = {
        "O": "не является членом предложения",
        "ADVERBIAL": "обстоятельство",
        "SUBJECT": "подлежащее",
        "PREDICATE": "сказуемое",
        "DEFINITION": "определение",
        "ADDITION": "дополнение",
    }

    print("\nDataset class distribution:")
    print("-" * 40)
    for class_name, count in sorted(
        label_counts.items(), key=lambda x: x[1], reverse=True
    ):
        display_name = class_names.get(class_name, class_name)
        print(f"  {display_name} ({class_name}): {count}")


def evaluate_model(
    test_dataset_path: str,
    model_path: Optional[str] = None,
    output_dir: str = "metrics_results",
    plot_cm: bool = True,
) -> Dict[str, Any]:
    """
    Основная функция для оценки модели.

    Args:
        test_dataset_path (str): Путь к файлу с тестовым датасетом
        model_path (Optional[str]): Путь к сохраненной модели.
            Если None, используется путь из конфигурации
        output_dir (str): Директория для сохранения результатов
        plot_cm (bool): Флаг построения матрицы ошибок

    Returns:
        Dict[str, Any]: Словарь с вычисленными метриками
    """
    _print_evaluation_header()

    texts, labels = _load_data(test_dataset_path)
    print_dataset_stats(texts, labels)

    predictor = _load_model(model_path)

    predictions, true_labels = collect_predictions(predictor, texts, labels)
    print(f"Total tokens processed: {len(predictions)}")

    calculator = MetricsCalculator()

    report = _generate_and_save_report(calculator, predictions, true_labels, output_dir)
    metrics = calculator.calculate_metrics(predictions, true_labels)

    print_metrics_summary(metrics)

    if plot_cm:
        _plot_and_save_confusion_matrix(
            calculator, predictions, true_labels, output_dir
        )

    save_results(metrics, report, output_dir)

    return metrics


def _print_evaluation_header() -> None:
    """Вывод заголовка оценки."""
    print("=" * 70)
    print("MODEL EVALUATION")
    print("=" * 70)


def _load_data(test_dataset_path: str) -> Tuple[List[str], List[List[str]]]:
    """
    Загрузка тестовых данных.

    Args:
        test_dataset_path (str): Путь к файлу с тестовым датасетом

    Returns:
        Tuple[List[str], List[List[str]]]: Кортеж из списка текстов и списка списков меток
    """
    print(f"\nLoading test dataset from {test_dataset_path}")
    texts, labels = DataProcessor.load_dataset(test_dataset_path)
    print(f"Loaded {len(texts)} samples")
    return texts, labels


def _load_model(model_path: Optional[str]) -> CircumstancePredictor:
    """
    Загрузка модели.

    Args:
        model_path (Optional[str]): Путь к сохраненной модели

    Returns:
        CircumstancePredictor: Объект предсказателя модели
    """
    print("\nLoading model...")
    return CircumstancePredictor(
        model_path=model_path or Config.MODEL_SAVE_PATH, debug=False
    )


def _generate_and_save_report(
    calculator: MetricsCalculator,
    predictions: List[int],
    true_labels: List[int],
    output_dir: str,
) -> str:
    """
    Генерация и сохранение отчета.

    Args:
        calculator (MetricsCalculator): Калькулятор метрик
        predictions (List[int]): Список ID предсказанных меток
        true_labels (List[int]): Список ID истинных меток
        output_dir (str): Директория для сохранения отчета

    Returns:
        str: Текст сгенерированного отчета
    """
    print("\nGenerating classification report...")
    report_path = os.path.join(output_dir, "classification_report.txt")
    return calculator.generate_report(predictions, true_labels, report_path)


def _plot_and_save_confusion_matrix(
    calculator: MetricsCalculator,
    predictions: List[int],
    true_labels: List[int],
    output_dir: str,
) -> None:
    """
    Построение и сохранение матрицы ошибок.

    Args:
        calculator (MetricsCalculator): Калькулятор метрик
        predictions (List[int]): Список ID предсказанных меток
        true_labels (List[int]): Список ID истинных меток
        output_dir (str): Директория для сохранения матрицы ошибок
    """
    print("\nGenerating confusion matrix...")
    cm = calculator.calculate_confusion_matrix(predictions, true_labels)
    cm_path = os.path.join(output_dir, "confusion_matrix.png")
    calculator.plot_confusion_matrix(cm, cm_path)

    cm_raw_path = os.path.join(output_dir, "confusion_matrix_raw.npy")
    np.save(cm_raw_path, cm)
    print(f"Raw confusion matrix saved to {cm_raw_path}")


def get_metrics(test_dataset_path: str) -> None:
    """
    Функция для получения метрик (вызывается из main.py).

    Args:
        test_dataset_path (str): Путь к файлу с тестовым датасетом
    """
    evaluate_model(test_dataset_path)
