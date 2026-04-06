// main.cpp
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "src/back/bert_onnx_inference.hpp"
#include "src/back/onnx_model.hpp"
#include "src/back/simple_tokenizer.hpp"

// Конфигурация модели
struct Config {
  std::string model_dir = "model";
  std::string vocab_path = "model/vocab.txt";
  std::string config_path = "model/config.json";
  std::string model_path = "model/bert_ner_model.onnx";
  size_t max_len = 128;
};

int main() {
  try {
    Config cfg;

    std::cout << "=== Анализатор членов предложения ===\n";
    std::cout << "Загрузка моделей...\n";

    // Загрузка компонентов
    auto labels = load_labels(cfg.config_path);
    SimpleTokenizer tokenizer(cfg.vocab_path);
    auto model = std::make_unique<onnx_infer::BertNerModel>(cfg.model_path);
    BertOnnxInference inferrer(std::move(model), tokenizer, labels,
                               cfg.max_len);

    std::cout << "Готово. Введите текст (пустая строка для выхода):\n\n";

    // Основной цикл
    std::string line;
    while (true) {
      std::cout << "> ";
      std::getline(std::cin, line);

      if (line.empty())
        break;

      // Анализ
      std::vector<SentenceResult> sentences =
          inferrer.extract_sentence_parts(line);

      // Вывод результатов
      for (const SentenceResult &sent : sentences) {
        std::cout << "\nПредложение: " << sent.text << "\n";

        if (sent.entities.empty()) {
          std::cout << "  Члены предложения не найдены\n";
          continue;
        }

        // Группировка по типам
        std::map<std::string, std::vector<std::string>> by_type;
        for (const Entity &e : sent.entities) {
          by_type[e.type_ru].push_back(e.text);
        }

        for (const auto &[type, texts] : by_type) {
          std::cout << "  " << type << ": ";
          for (size_t i = 0; i < texts.size(); ++i) {
            std::cout << '"' << texts[i] << '"';
            if (i < texts.size() - 1)
              std::cout << ", ";
          }
          std::cout << "\n";
        }
      }
      std::cout << "\n";
    }

  } catch (const std::exception &e) {
    std::cerr << "Ошибка: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
