#include "inference.h"
#include <vector>

/**
 * @brief Конструктор
 *
 * @param model ONNX модель
 * @param tokenizer Токенизатор
 * @param labels Метки (ID -> {английское_название, русское_название})
 * @param max_len Максимальная длина последовательности
 */
Inference::Inference(
    std::unique_ptr<onnx_infer::BertNerModel> model, SimpleTokenizer &tokenizer,
    const std::map<int, std::pair<std::string, std::string>> &labels,
    size_t max_len)
    : model_(std::move(model)), tokenizer_(tokenizer), labels_(labels),
      max_len_(max_len) {}

/**
 * @brief Обрабатать одно предложение
 *
 * @param sentence Текст предложения
 * @return SentenceResult Результат обработки
 */
Inference::ProcessedSentence
Inference::process_sentence(const std::string &sentence) {
  ProcessedSentence result;
  result.sentence = sentence;

  SimpleTokenizer::EncodingResult encoding =
      tokenizer_.encode(sentence, max_len_);
  if (encoding.input_ids.empty()) {
    return result;
  }

  std::vector<int> predictions =
      model_->predict_labels(encoding.input_ids, encoding.attention_mask);

  result.tokens = encoding.tokens;
  result.token_labels = predictions;
  result.entities =
      merge_subwords(encoding.tokens, predictions, encoding.offsets, sentence);

  return result;
}

/**
 * @brief Объединить подслова в полные слова с метками
 *
 * @param tokens Токены
 * @param token_labels Метки токенов
 * @param offsets Смещения токенов
 * @param original_text Исходный текст
 * @return std::vector<Entity> Объединенные сущности
 */
std::vector<Inference::Entity>
merge_subwords(const std::vector<std::string> &tokens,
               const std::vector<int> &token_labels,
               const std::vector<std::pair<size_t, size_t>> &offsets,
               const std::string &original_text) {
  std::vector<Inference::Entity> entities;

  /*
   * для каждого токена:
   *  если токен подслово и текущее слово не закончено и не None:
   *    добавить в текущее слово
   *  иначе:
   *    закончить слово
   *    начать новое слово
   *  закончить текущее слово
   */

  return entities;
}
