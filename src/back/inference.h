#ifndef INFERENCE_H
#define INFERENCE_H

#include <cstddef>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "onnx_model.h"
#include "simple_tokenizer.h"

class Inference {

public:
  /**
   * @brief Информация о каждом токене
   */
  struct Token {
    std::string clean_token; // текст токена без префикса
    std::string full_token;  // текст токена с префиксом
    std::string label;       // лейбл токена
    bool is_subword;         // является ли подсловом
    size_t position;         // позиция токена в последовательности
  };

  /**
   * @brief Слово, составленное из токенов
   */
  struct Word {
    std::string text;                // слово
    size_t start;                    // начальная позиция в тексте
    size_t end;                      // конечная позиция в тексте
    std::string labels;              // лейблы токенов, входящих в слово
    std::string main_label = "O";    // основная метка слова (B- -> I- -> O)
    bool has_b_prefix = false;       // есть ли среди лейблов токенов B-
    std::vector<std::string> tokens; // токены, составляющие слово
  };

  /**
   * @brief Сущность (член предложения), найденная в тексте
   */
  struct Entity {
    std::string sentence_part_type;     // тип члена предложения на английском
    std::string sentence_part_type_rus; // тип члена предложения на русском
    std::string text;                   // текст члена предложения
    size_t start;                       // начальная позиция в тексте
    size_t end;                         // конечная позиция в тексте
    float confidence; // мера принадлежности к классу этого члена
                      // предложения
  };

  /**
   * @brief Результат обработки одного предложения
   */
  struct ProcessedSentence {
    std::string sentence;            // текст предложения
    std::vector<Entity> entities;    // члены предложения
    std::vector<std::string> tokens; // токены
    std::vector<int> token_labels;   // лейблы токенов
  };

  /**
   * @brief Конструктор
   *
   * @param model ONNX модель
   * @param tokenizer Токенизатор
   * @param labels Метки (ID -> {английское_название, русское_название})
   * @param max_len Максимальная длина последовательности
   */
  Inference(std::unique_ptr<onnx_infer::BertNerModel> model,
            SimpleTokenizer &tokenizer,
            const std::map<int, std::pair<std::string, std::string>> &labels,
            const size_t max_len = 128);

  /**
   * @brief Обрабатать одно предложение
   *
   * @param sentence Текст предложения
   * @return SentenceResult Результат обработки
   */
  ProcessedSentence process_sentence(const std::string &sentence);

  /**
   * @brief Извлечь члены предложения из текста
   *
   * @param text Входной текст
   * @return std::vector<SentenceResult> Результаты по предложениям
   */
  std::vector<ProcessedSentence> process_text(const std::string &text);

protected:
  std::unique_ptr<onnx_infer::BertNerModel> model_; // модель
  SimpleTokenizer &tokenizer_;                      // ссылка на токенизатор
  const size_t max_len_; // максимальная длина предложения
  const std::map<int, std::pair<std::string, std::string>>
      labels_; // карта лейблов

  /**
   * @brief Объединить подслова в полные слова с метками
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

  /**
   * @brief Дополнить текущее слово
   * @param word информация о слове
   * @param token текст токена
   * @param end TODO: описание
   */
  void extend_current_word(Word &word, std::string token, size_t end);

  /*
   * @brief Завершить слово и добавить его в массив слов
   * @param words вектор слов
   * @param word информация о слове
   */
  void finalize_current_word(std::vector<Word> words, Word word);

  /*
   * @brief Начать новое слово
   * @param new_word ссылка на новое слово
   * @param token текст токена
   * @param label лейбл токена
   * @param start начальная позиция в тексте
   * @param end конечная позиция в тексте
   */
  void start_new_word(Word &new_word, std::string token, std::string label,
                      size_t start, size_t end);
};

#endif // !INFERENCE_H
