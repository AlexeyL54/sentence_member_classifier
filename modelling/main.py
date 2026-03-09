import argparse
import os
from config import Config


def train_model():
    """Обучение модели"""
    from train import main as train_main

    train_main()


def predict_interactive():
    """Интерактивный режим предсказания"""
    from inference import CircumstancePredictor

    try:
        predictor = CircumstancePredictor()
        print("✓ Model loaded successfully")
        print("Type 'exit' to quit\n")
    except Exception as e:
        print(f"✗ Error loading model: {e}")
        return

    while True:
        try:
            text = input("\nEnter text: ").strip()

            if text.lower() in ["exit", "quit", "q"]:
                break

            if not text:
                continue

            results = predictor.extract_circumstances(text)

            print("\n" + "=" * 60)
            for result in results:
                print(f"\nSentence: {result['sentence']}")
                if result["entities"]:
                    print("Circumstances found:")
                    for entity in result["entities"]:
                        print(f"  • '{entity['text']}' ({entity['type']})")
                else:
                    print("No circumstances found")
            print("=" * 60)

        except KeyboardInterrupt:
            print("\n\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")


def predict_file(input_path, output_path):
    """Обработка файла"""
    from inference import CircumstancePredictor

    try:
        predictor = CircumstancePredictor()
    except Exception as e:
        print(f"Error loading model: {e}")
        return

    if not os.path.exists(input_path):
        print(f"Input file not found: {input_path}")
        return

    try:
        predictor.process_file(input_path, output_path)
    except Exception as e:
        print(f"Error processing file: {e}")


def list_labels():
    """Вывод списка меток"""
    print("Supported labels:")
    print("-" * 30)
    for label in Config.LABEL_LIST:
        print(f"  {label}")
    print(f"\nTotal labels: {Config.NUM_LABELS}")


def show_info():
    """Информация о проекте"""
    print("=" * 60)
    print("CIRCUMSTANCE DETECTION WITH BERT")
    print("=" * 60)
    print(f"Model: {Config.MODEL_NAME}")
    print(f"Labels: {Config.NUM_LABELS}")
    print(f"Device: {Config.DEVICE}")
    print(f"Model path: {Config.MODEL_SAVE_PATH}")
    print(f"ONNX path: {Config.ONNX_SAVE_PATH}")
    print(f"Dataset path: {Config.DATASET_PATH}")
    print("=" * 60)


def analyze_structure():
    """Анализ структуры предложения"""
    from inference import CircumstancePredictor

    try:
        predictor = CircumstancePredictor()
        print("✓ Model loaded successfully")
        print("Type 'exit' to quit\n")
    except Exception as e:
        print(f"✗ Error loading model: {e}")
        return

    while True:
        try:
            text = input("\nEnter sentence: ").strip()

            if text.lower() in ["exit", "quit", "q"]:
                break

            if not text:
                continue

            structure = predictor.analyze_sentence_structure(text)

            print("\n" + "=" * 60)
            print("SENTENCE STRUCTURE ANALYSIS")
            print("=" * 60)
            print(f"Sentence: {structure['sentence']}\n")

            if structure["main_parts"]["subject"]:
                print("Подлежащее:")
                for subj in structure["main_parts"]["subject"]:
                    print(f"  • {subj['text']}")

            if structure["main_parts"]["predicate"]:
                print("\nСказуемое:")
                for pred in structure["main_parts"]["predicate"]:
                    print(f"  • {pred['text']}")

            if structure["secondary_parts"]["object"]:
                print("\nДополнение:")
                for obj in structure["secondary_parts"]["object"]:
                    print(f"  • {obj['text']}")

            if structure["secondary_parts"]["attribute"]:
                print("\nОпределение:")
                for attr in structure["secondary_parts"]["attribute"]:
                    print(f"  • {attr['text']}")

            if structure["circumstances"]:
                print("\nОбстоятельства:")
                for circ in structure["circumstances"]:
                    print(f"  • {circ['text']} ({circ['type']})")

            print("=" * 60)

        except KeyboardInterrupt:
            print("\n\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")


def get_dataset_stats():
    """Получение статистики по датасету"""
    from dataset import DataProcessor
    from config import Config

    try:
        texts, labels = DataProcessor.load_dataset(Config.DATASET_PATH)
        DataProcessor.get_statistics(texts, labels)
    except Exception as e:
        print(f"Error loading dataset: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="BERT-based Sentence Part Detection",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --train                    Train the model
  %(prog)s --predict                  Interactive prediction mode
  %(prog)s --analyze                  Analyze sentence structure
  %(prog)s --file input.txt           Process text file
  %(prog)s --stats                    Show dataset statistics
  %(prog)s --labels                   List all supported labels
  %(prog)s --info                     Show project information
        """,
    )

    parser.add_argument("--train", action="store_true", help="Train the model")
    parser.add_argument(
        "--predict", action="store_true", help="Interactive prediction mode"
    )
    parser.add_argument(
        "--analyze", action="store_true", help="Analyze sentence structure"
    )
    parser.add_argument("--file", type=str, help="Process text file")
    parser.add_argument("--output", type=str, help="Output file for processing results")
    parser.add_argument(
        "--labels", action="store_true", help="List all supported labels"
    )
    parser.add_argument("--stats", action="store_true", help="Show dataset statistics")
    parser.add_argument("--info", action="store_true", help="Show project information")

    args = parser.parse_args()

    if args.train:
        train_model()
    elif args.predict:
        predict_interactive()
    elif args.analyze:
        analyze_structure()
    elif args.file:
        predict_file(args.file, args.output)
    elif args.labels:
        list_labels()
    elif args.stats:
        get_dataset_stats()
    elif args.info:
        show_info()
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
