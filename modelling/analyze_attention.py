"""
Скрипт для визуализации механизмов внимания (Attention Heads) модели BERT.
"""

import torch
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np
from transformers import AutoTokenizer
from model import CircumstanceBERT
from config import Config
from typing import List


def visualize_attention(
    tokens: List[str],
    attention_weights: torch.Tensor,
    layer_idx: int = -1,
    head_idx: int = 0,
    save_path: str = "attention_map.png",
):
    """
    Визуализирует матрицу внимания для конкретного слоя и головы.

    Args:
        tokens: Список токенов (слов)
        attention_weights: Тензор весов внимания [num_layers, batch, num_heads, seq_len, seq_len]
        layer_idx: Индекс слоя (например, -1 для последнего)
        head_idx: Индекс головы внимания (0-11 для base модели)
    """
    # Извлекаем веса для конкретного слоя и головы
    # attention_weights shape: (num_layers, batch_size, num_heads, seq_len, seq_len)
    weights = attention_weights[layer_idx][0][head_idx].cpu().numpy()

    # Убираем специальные токены [CLS] и [SEP] для чистоты картинки, если нужно
    # Но обычно интереснее смотреть на всю матрицу, включая их

    plt.figure(figsize=(12, 10))
    sns.heatmap(weights, xticklabels=tokens, yticklabels=tokens, cmap="viridis")
    plt.title(f"Attention Map: Layer {layer_idx}, Head {head_idx}")
    plt.xlabel("Key Tokens")
    plt.ylabel("Query Tokens")
    plt.xticks(rotation=90)
    plt.yticks(rotation=0)
    plt.tight_layout()
    plt.savefig(save_path)
    print(f"Attention map saved to {save_path}")
    plt.show()


def analyze_sentence(sentence: str):
    """
    Загружает модель и анализирует внимание для одного предложения.
    """
    # 1. Загрузка токенизатора и модели
    tokenizer = AutoTokenizer.from_pretrained(Config.MODEL_NAME)

    # Важно: загружаем нашу кастомную модель, а не стандартную AutoModel
    model = CircumstanceBERT.from_pretrained(Config.MODEL_SAVE_PATH)
    model.to(Config.DEVICE)  # type: ignore
    model.eval()

    # 2. Токенизация
    inputs = tokenizer(
        sentence,
        return_tensors="pt",
        padding=True,
        truncation=True,
        max_length=Config.MAX_LEN,
    )
    input_ids = inputs["input_ids"].to(Config.DEVICE)
    attention_mask = inputs["attention_mask"].to(Config.DEVICE)

    # Получаем токены для подписей осей
    tokens = tokenizer.convert_ids_to_tokens(input_ids[0])

    # 3. Прямой проход с получением внимания
    with torch.no_grad():
        # Используем наш новый метод
        _, _, attentions = model.forward_with_attentions(
            input_ids=input_ids, attention_mask=attention_mask
        )

    # attentions - это кортеж из 12 тензоров (для ruBert-base)
    # Каждый тензор имеет форму: [batch_size, num_heads, seq_len, seq_len]
    print(f"Number of layers: {len(attentions)}")
    print(f"Shape of attention in last layer: {attentions[-1].shape}")

    # 4. Визуализация
    # Попробуем визуализировать последнюю голову последнего слоя
    # Обычно последние слои лучше捕捉ают синтаксические связи
    visualize_attention(
        tokens=tokens,
        attention_weights=attentions,
        layer_idx=-1,  # Последний слой
        head_idx=0,  # Первая голова
        save_path="attention_layer_last_head_0.png",
    )

    # Можно также посмотреть на среднее внимание по всем головам
    # attentions[-1] имеет форму [batch, heads, seq, seq]
    # Берем mean по dim=1 (heads), чтобы получить [batch, seq, seq]
    # Затем берем [0] для первого элемента батча
    avg_attention = attentions[-1].mean(dim=1)[0].cpu().numpy()

    plt.figure(figsize=(12, 10))
    sns.heatmap(avg_attention, xticklabels=tokens, yticklabels=tokens, cmap="inferno")
    plt.title("Average Attention Across All Heads (Last Layer)")
    plt.xticks(rotation=90)
    plt.tight_layout()
    plt.savefig("attention_avg_last_layer.png")
    plt.show()


if __name__ == "__main__":
    # Пример предложения из вашего датасета
    test_sentence = "Демидов Руслан Остапович изучил репутацию компании."
    print(f"Analyzing sentence: '{test_sentence}'")
    analyze_sentence(test_sentence)
