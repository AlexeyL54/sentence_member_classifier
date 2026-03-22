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
        """Инициализация калькулятора метрик."""
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
        """Получение базового класса из BIO-метки."""
        if label == "O":
            return "O"
        if "-" in label:
            return label.split("-")[1]
        return label

    def convert_to_base_ids(
        self, predictions: List[int], true_labels: List[int]
    ) -> Tuple[List[int], List[int]]:
        """Преобразование BIO-меток в ID базовых классов."""
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
        """Расчет precision, recall, f1 с weighted average."""
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
        """Расчет метрик для каждого класса."""
        labels = list(range(len(self.base_classes)))
        result = precision_recall_fscore_support(
            true_labels, predictions, labels=labels, zero_division=0
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
        for i, class_name in enumerate(self.base_classes):
            display_name = self.class_names[class_name]
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
        """Генерация отчета о классификации."""
        pred_base, true_base = self.convert_to_base_ids(predictions, true_labels)
        target_names = [self.class_names[c] for c in self.base_classes]

        report = classification_report(
            true_base, pred_base, target_names=target_names, zero_division=0
        )

        full_report = self._format_report_header(len(pred_base)) + report

        if output_file:
            self._save_report(full_report, output_file)

        return full_report

    def _format_report_header(self, total_tokens: int) -> str:
        """Форматирование заголовка отчета."""
        header = f"\n{'=' * 70}\n"
        header += f"CLASSIFICATION REPORT\n"
        header += f"{'=' * 70}\n"
        header += f"Total tokens evaluated: {total_tokens}\n"
        header += f"{'=' * 70}\n\n"
        return header

    def _save_report(self, report: str, output_file: str) -> None:
        """Сохранение отчета в файл."""
        os.makedirs(os.path.dirname(output_file), exist_ok=True)
        with open(output_file, "w", encoding="utf-8") as f:
            f.write(report)
        print(f"Report saved to {output_file}")

    def calculate_confusion_matrix(
        self, predictions: List[int], true_labels: List[int]
    ) -> np.ndarray:
        """Расчет матрицы ошибок."""
        pred_base, true_base = self.convert_to_base_ids(predictions, true_labels)
        return confusion_matrix(
            true_base, pred_base, labels=list(range(len(self.base_classes)))
        )

    def plot_confusion_matrix(
        self, cm: np.ndarray, output_path: Optional[str] = None
    ) -> None:
        """Визуализация матрицы ошибок."""
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
        """Создание графика матрицы ошибок."""
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
        """Добавление значений в ячейки матрицы с корректным округлением."""
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
        """Сохранение графика."""
        os.makedirs(os.path.dirname(output_path), exist_ok=True)
        plt.savefig(output_path, dpi=300, bbox_inches="tight")
        print(f"Confusion matrix saved to {output_path}")


def collect_predictions(
    predictor: CircumstancePredictor,
    texts: List[str],
    labels: List[List[str]],
) -> Tuple[List[int], List[int]]:
    """Сбор предсказаний модели для всех текстов."""
    all_predictions = []
    all_true_labels = []

    for text, true_seq in zip(texts, labels):
        tokens, offsets = predictor.predict_sentence(text)
        pred_ids = _get_prediction_ids(tokens)
        true_ids = _align_true_labels(tokens, true_seq)

        pred_ids, true_ids = _align_lengths(pred_ids, true_ids)

        all_predictions.extend(pred_ids)
        all_true_labels.extend(true_ids)

    return all_predictions, all_true_labels


def _get_prediction_ids(tokens: List) -> List[int]:
    """Получение ID предсказанных меток."""
    pred_ids = []
    for token in tokens:
        label_id = Config.LABEL2ID.get(token.label, Config.LABEL2ID["O"])
        pred_ids.append(label_id)
    return pred_ids


def _align_true_labels(tokens: List, true_seq: List[str]) -> List[int]:
    """Выравнивание истинных меток с токенами."""
    true_ids = []
    for i in range(len(tokens)):
        if i < len(true_seq):
            true_ids.append(Config.LABEL2ID.get(true_seq[i], Config.LABEL2ID["O"]))
        else:
            true_ids.append(Config.LABEL2ID["O"])
    return true_ids


def _align_lengths(
    pred_ids: List[int], true_ids: List[int]
) -> Tuple[List[int], List[int]]:
    """Выравнивание длин предсказаний и истинных меток."""
    if len(true_ids) < len(pred_ids):
        true_ids.extend([Config.LABEL2ID["O"]] * (len(pred_ids) - len(true_ids)))
    else:
        true_ids = true_ids[: len(pred_ids)]
        pred_ids = pred_ids[: len(true_ids)]
    return pred_ids, true_ids


def print_metrics_summary(metrics: Dict[str, Any]) -> None:
    """Вывод сводки метрик в консоль."""
    _print_aggregated_metrics(metrics)
    _print_per_class_metrics(metrics)
    _print_class_comparison(metrics)


def _print_aggregated_metrics(metrics: Dict[str, Any]) -> None:
    """Вывод агрегированных метрик."""
    print("\n" + "=" * 70)
    print("AGGREGATED METRICS (Weighted Average)")
    print("=" * 70)
    print(f"Precision: {metrics['precision']:.4f}")
    print(f"Recall: {metrics['recall']:.4f}")
    print(f"F1-score: {metrics['f1']:.4f}")
    print(f"Total support: {metrics['support']}")


def _print_per_class_metrics(metrics: Dict[str, Any]) -> None:
    """Вывод метрик для каждого класса."""
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
    """Вывод сравнения между членами предложения."""
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
    """Сохранение результатов оценки в файлы."""
    os.makedirs(output_dir, exist_ok=True)

    results = _prepare_results_json(metrics, report)

    json_path = os.path.join(output_dir, "evaluation_results.json")
    with open(json_path, "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=2)
    print(f"\nResults saved to {json_path}")


def _prepare_results_json(metrics: Dict[str, Any], report: str) -> Dict[str, Any]:
    """Подготовка данных для JSON."""
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
    """Вывод статистики по датасету."""
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
    """Основная функция для оценки модели."""
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
    """Загрузка тестовых данных."""
    print(f"\nLoading test dataset from {test_dataset_path}")
    texts, labels = DataProcessor.load_dataset(test_dataset_path)
    print(f"Loaded {len(texts)} samples")
    return texts, labels


def _load_model(model_path: Optional[str]) -> CircumstancePredictor:
    """Загрузка модели."""
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
    """Генерация и сохранение отчета."""
    print("\nGenerating classification report...")
    report_path = os.path.join(output_dir, "classification_report.txt")
    return calculator.generate_report(predictions, true_labels, report_path)


def _plot_and_save_confusion_matrix(
    calculator: MetricsCalculator,
    predictions: List[int],
    true_labels: List[int],
    output_dir: str,
) -> None:
    """Построение и сохранение матрицы ошибок."""
    print("\nGenerating confusion matrix...")
    cm = calculator.calculate_confusion_matrix(predictions, true_labels)
    cm_path = os.path.join(output_dir, "confusion_matrix.png")
    calculator.plot_confusion_matrix(cm, cm_path)

    cm_raw_path = os.path.join(output_dir, "confusion_matrix_raw.npy")
    np.save(cm_raw_path, cm)
    print(f"Raw confusion matrix saved to {cm_raw_path}")


def get_metrics(test_dataset_path: str) -> None:
    """Функция для получения метрик (вызывается из main.py)."""
    evaluate_model(test_dataset_path)
