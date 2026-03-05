# prepare_tokenizer.py
from transformers import AutoTokenizer
import json
import os


def export_tokenizer(model_name="DeepPavlov/rubert-base-cased", output_dir="models"):
    """Экспорт токенизатора для C++ программы"""
    tokenizer = AutoTokenizer.from_pretrained(model_name)

    # Сохраняем в формате JSON
    tokenizer.save_pretrained(output_dir)

    # Копируем vocab.txt если его нет
    vocab_path = os.path.join(output_dir, "vocab.txt")
    if not os.path.exists(vocab_path):
        with open(vocab_path, "w", encoding="utf-8") as f:
            vocab = tokenizer.get_vocab()
            sorted_vocab = sorted(vocab.items(), key=lambda x: x[1])
            for token, _ in sorted_vocab:
                f.write(token + "\n")

    print(f"Tokenizer exported to {output_dir}")
    print(f"Files created:")
    print(f"  - tokenizer.json (main config)")
    print(f"  - vocab.txt")
    print(f"  - tokenizer_config.json")


if __name__ == "__main__":
    export_tokenizer()
