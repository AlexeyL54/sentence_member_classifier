#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "onnx_model.hpp"
#include "simple_tokenizer.hpp"

/**
 * @brief Сущность (член предложения), найденная в тексте
 */
struct Entity {
  std::string text;    // Текст сущности
  std::string type;    // Тип на английском (например, "B-SUBJECT")
  std::string type_ru; // Тип на русском (например, "Подлежащее")
  size_t start;        // Начальная позиция в тексте
  size_t end;          // Конечная позиция в тексте
  // float confidence;    // Уверенность модели
};

/**
 * @brief Результат обработки одного предложения
 */
struct SentenceResult {
  std::string text;                // Исходный текст предложения
  std::vector<Entity> entities;    // Найденные сущности
  std::vector<std::string> tokens; // Токены
  std::vector<int> token_labels;   // Метки для каждого токена
  std::uint16_t err = 0;           // код ошибки обработки предложения
};

std::map<int, std::pair<std::string, std::string>>
load_labels(const std::string &path);

/**
 * @brief Класс для объединения токенизатора и ONNX модели для NER
 */
class BertOnnxInference {
public:
  /**
   * @brief Конструктор
   *
   * @param model ONNX модель
   * @param tokenizer Токенизатор
   * @param labels Метки (ID -> {английское_название, русское_название})
   * @param max_len Максимальная длина последовательности
   */
  BertOnnxInference(
      std::unique_ptr<onnx_infer::BertNerModel> model,
      std::shared_ptr<SimpleTokenizer> tokenizer,
      const std::map<int, std::pair<std::string, std::string>> &labels,
      size_t max_len = 128);

  /**
   * @brief Извлекает члены предложения из текста
   *
   * @param text Входной текст
   * @return std::vector<SentenceResult> Результаты по предложениям
   */
  std::vector<SentenceResult> extract_sentence_parts(const std::string &text);

  /**
   * @brief Обрабатывает одно предложение
   *
   * @param sentence Текст предложения
   * @return SentenceResult Результат обработки
   */
  SentenceResult process_sentence(const std::string &sentence);

  /**
   * @brief Разбивает текст на предложения
   *
   * @param text Входной текст
   * @return std::vector<std::string> Предложения
   */
  std::vector<std::string> split_into_sentences(const std::string &text);

private:
  std::unique_ptr<onnx_infer::BertNerModel> model_;
  std::shared_ptr<SimpleTokenizer> tokenizer_;
  std::map<int, std::pair<std::string, std::string>> labels_;
  size_t max_len_;

  /**
   * @brief Объединяет подслова в полные слова с метками
   *
   * @param tokens Токены
   * @param token_labels Метки токенов
   * @param offsets Смещения токенов
   * @param original_text Исходный текст
   * @return std::vector<Entity> Объединенные сущности
   */
  std::vector<Entity>
  merge_subwords(const std::vector<std::string> &tokens,
                 const std::vector<int> &token_labels,
                 const std::vector<std::pair<size_t, size_t>> &offsets,
                 const std::string &original_text);
};
