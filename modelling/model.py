"""
Модуль с определением архитектуры модели для распознавания обстоятельств.

Содержит класс CircumstanceBERT, который наследуется от PreTrainedModel
и реализует BERT-based модель с опциональным CRF слоем для NER задач.
"""

import torch
import torch.nn as nn
from transformers import AutoModel, PreTrainedModel, BertConfig
from config import Config


class CircumstanceBERT(PreTrainedModel):
    """
    Модель для распознавания обстоятельств на основе BERT.

    Реализует архитектуру трансформера с классификационным слоем
    для токенов и опциональным CRF слоем для улучшения качества
    последовательностей меток.

    Attributes:
        num_labels (int): Количество классов меток
        use_crf (bool): Флаг использования CRF слоя
        bert: Базовая BERT модель
        dropout: Dropout слой для регуляризации
        classifier: Линейный классификатор для токенов
        crf: CRF слой (опционально)
    """

    config_class = BertConfig

    def __init__(self, config):
        """
        Инициализация модели CircumstanceBERT.

        Args:
            config: Конфигурация модели от transformers
        """
        super().__init__(config)
        self.num_labels = Config.NUM_LABELS
        self.use_crf = Config.USE_CRF

        # BERT модель
        self.bert = AutoModel.from_config(config)

        # Dropout
        self.dropout = nn.Dropout(Config.DROPOUT)

        # Классификатор для NER
        self.classifier = nn.Linear(config.hidden_size, Config.NUM_LABELS)

        # CRF слой (опционально)
        if self.use_crf:
            try:
                from torchcrf import CRF

                self.crf = CRF(Config.NUM_LABELS, batch_first=True)
            except ImportError:
                print("Warning: pytorch-crf not installed. Using softmax instead.")
                self.use_crf = False

        self.post_init()

    def forward(
        self,
        input_ids=None,
        attention_mask=None,
        token_type_ids=None,
        labels=None,
        **kwargs,
    ):
        """
        Прямой проход через модель.

        Args:
            input_ids (torch.Tensor, optional): Тензор с ID токенов [batch_size, seq_len]
            attention_mask (torch.Tensor, optional): Маска внимания [batch_size, seq_len]
            token_type_ids (torch.Tensor, optional): ID типов токенов [batch_size, seq_len]
            labels (torch.Tensor, optional): Истинные метки для вычисления потерь [batch_size, seq_len]
            **kwargs: Дополнительные аргументы для BERT модели

        Returns:
            tuple:
                - loss (torch.Tensor, optional): Значение функции потерь (если переданы labels)
                - logits (torch.Tensor): Логиты модели [batch_size, seq_len, num_labels]
        """
        if attention_mask is None:
            attention_mask = torch.ones_like(input_ids)  # type: ignore

        outputs = self.bert(
            input_ids=input_ids,
            attention_mask=attention_mask,
            token_type_ids=token_type_ids,
            **kwargs,
        )

        sequence_output = outputs[0]
        sequence_output = self.dropout(sequence_output)
        logits = self.classifier(sequence_output)  # [batch, seq_len, num_labels]

        loss = None
        if labels is not None:
            if self.use_crf and hasattr(self, "crf"):
                # CRF loss
                mask = attention_mask.bool()
                # CRF ожидает logits в формате [batch, seq_len, num_labels]
                # и labels в формате [batch, seq_len]
                loss = -self.crf(logits, labels, mask=mask, reduction="mean")
            else:
                # Обычный cross entropy loss
                loss_fct = nn.CrossEntropyLoss(ignore_index=-100)

                # Активные токены (не паддинг)
                active_loss = attention_mask.view(-1) == 1
                active_logits = logits.view(-1, self.num_labels)
                active_labels = torch.where(
                    active_loss,
                    labels.view(-1),
                    torch.tensor(loss_fct.ignore_index).type_as(labels),
                )
                loss = loss_fct(active_logits, active_labels)

        return (loss, logits) if loss is not None else (logits,)

    def forward_with_attentions(
        self,
        input_ids=None,
        attention_mask=None,
        token_type_ids=None,
        labels=None,
    ):
        """
        Прямой проход с возвратом весов внимания.
        Используется только для анализа/отладки, так как потребляет больше памяти.
        """
        if attention_mask is None:
            attention_mask = torch.ones_like(input_ids)  # type: ignore

        # ВАЖНО: output_attentions=True заставляет BERT вернуть веса внимания
        outputs = self.bert(
            input_ids=input_ids,
            attention_mask=attention_mask,
            token_type_ids=token_type_ids,
            output_attentions=True,
        )

        sequence_output = outputs[0]  # [batch, seq_len, hidden_size]
        attentions = outputs[
            -1
        ]  # Tuple of tensors: [layer][batch, head, seq_len, seq_len]

        sequence_output = self.dropout(sequence_output)
        logits = self.classifier(sequence_output)

        loss = None
        if labels is not None:
            if self.use_crf and hasattr(self, "crf"):
                mask = attention_mask.bool()
                loss = -self.crf(logits, labels, mask=mask, reduction="mean")
            else:
                loss_fct = nn.CrossEntropyLoss(ignore_index=-100)
                active_loss = attention_mask.view(-1) == 1
                active_logits = logits.view(-1, self.num_labels)
                active_labels = torch.where(
                    active_loss,
                    labels.view(-1),
                    torch.tensor(loss_fct.ignore_index).type_as(labels),
                )
                loss = loss_fct(active_logits, active_labels)

        return loss, logits, attentions

    def predict(self, logits, attention_mask):
        """
        Получение предсказаний из логитов модели.

        Args:
            logits (torch.Tensor): Логиты модели [batch_size, seq_len, num_labels]
            attention_mask (torch.Tensor): Маска внимания [batch_size, seq_len]

        Returns:
            torch.Tensor: Тензор с предсказанными метками [batch_size, seq_len]
        """
        if self.use_crf and hasattr(self, "crf"):
            mask = attention_mask.bool()
            predictions = self.crf.decode(logits, mask=mask)
            return torch.tensor(predictions)
        else:
            return torch.argmax(logits, dim=-1)
