# export_vocab.py
from transformers import AutoTokenizer
from config import Config
import os


def export_bert_vocab(model_name=Config.MODEL_NAME, output_path=Config.VOCAB_PATH):
    """Экспорт словаря BERT в файл"""
    tokenizer = AutoTokenizer.from_pretrained(model_name)

    # Получаем словарь
    vocab = tokenizer.get_vocab()

    # Сортируем по ID
    sorted_vocab = sorted(vocab.items(), key=lambda x: x[1])

    # Сохраняем в файл
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, "w", encoding="utf-8") as f:
        for token, _ in sorted_vocab:
            f.write(token + "\n")

    print(f"Vocabulary exported to {output_path}")
    print(f"Total tokens: {len(sorted_vocab)}")

    # Проверяем специальные токены
    print("\nSpecial tokens:")
    print(f"  [PAD]: {tokenizer.pad_token} (ID: {tokenizer.pad_token_id})")
    print(f"  [UNK]: {tokenizer.unk_token} (ID: {tokenizer.unk_token_id})")
    print(f"  [CLS]: {tokenizer.cls_token} (ID: {tokenizer.cls_token_id})")
    print(f"  [SEP]: {tokenizer.sep_token} (ID: {tokenizer.sep_token_id})")
    print(f"  [MASK]: {tokenizer.mask_token} (ID: {tokenizer.mask_token_id})")


if __name__ == "__main__":
    export_bert_vocab(model_name=Config.MODEL_NAME, output_path=Config.VOCAB_PATH)
