import torch
import torch.nn as nn
from transformers import AutoModel, PreTrainedModel
from config import Config


class CircumstanceBERT(PreTrainedModel):
    def __init__(self, config):
        super().__init__(config)
        self.num_labels = Config.NUM_LABELS

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
        # Создаем mask для внимания
        if attention_mask is None:
            attention_mask = torch.ones_like(input_ids)
        
        outputs = self.bert(
            input_ids=input_ids,
            attention_mask=attention_mask,
            token_type_ids=token_type_ids,
            **kwargs,
        )

        sequence_output = outputs[0]
        sequence_output = self.dropout(sequence_output)
        logits = self.classifier(sequence_output)

        loss = None
        if labels is not None:
            if self.use_crf:
                # CRF loss
                mask = attention_mask.bool()
                loss = -self.crf(logits, labels, mask=mask, reduction="mean")
            else:
                # Обычный cross entropy loss
                loss_fct = nn.CrossEntropyLoss(ignore_index=-100)
                
                # Активные токены (не паддинг)
                active_loss = attention_mask.view(-1) == 1
                active_logits = logits.view(-1, self.num_labels)
                
                # Подготавливаем метки
                active_labels = torch.where(
                    active_loss,
                    labels.view(-1),
                    torch.tensor(loss_fct.ignore_index).type_as(labels)
                )
                
                loss = loss_fct(active_logits, active_labels)

        output = (logits,)
        if loss is not None:
            output = (loss,) + output
            
        return output

    def predict(self, logits, attention_mask):
        if self.use_crf and hasattr(self, 'crf'):
            mask = attention_mask.bool()
            predictions = self.crf.decode(logits, mask=mask)
            return torch.tensor(predictions)
        else:
            # Просто берем argmax
            return torch.argmax(logits, dim=-1)
