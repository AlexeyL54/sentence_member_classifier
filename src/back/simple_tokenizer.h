// simple_tokenizer.h
#ifndef SIMPLE_TOKENIZER_H
#define SIMPLE_TOKENIZER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief Простой токенизатор для BERT моделей с поддержкой WordPiece
 *
 * Реализует токенизацию текста с использованием словаря в формате BERT.
 * Поддерживает UTF-8, включая кириллицу, и специальные токены [CLS], [SEP],
 * [PAD], [UNK]. Использует алгоритм WordPiece для разбиения слов на подслова.
 */
class SimpleTokenizer {
public:
  /**
   * @brief Результат кодирования с полной информацией для NER
   *
   * Содержит все необходимые данные для передачи в модель и пост-обработки.
   */
  struct EncodingResult {
    std::vector<int64_t> input_ids;      // ID токенов в словаре
    std::vector<int64_t> attention_mask; // Маска внимания (1 для реальных
                                         // токенов, 0 для паддинга)
    std::vector<std::pair<size_t, size_t>>
        offsets; // Смещения в исходном тексте (начало, конец)
    std::vector<std::string> tokens; // Строковые представления токенов
    std::vector<int> word_ids;       // ID слов для группировки подслов
  };

  /**
   * @brief Результат токенизации с базовой информацией
   *
   * Упрощенная версия для случаев, когда не требуется группировка по словам.
   */
  struct TokenizationResult {
    std::vector<int64_t> input_ids;      // ID токенов в словаре
    std::vector<int64_t> attention_mask; // Маска внимания
    std::vector<std::string> tokens;     // Строковые представления токенов
    std::vector<std::pair<size_t, size_t>>
        offsets; // Смещения в исходном тексте
  };

  /**
   * @brief Конструктор токенизатора
   *
   * @param vocab_path Путь к файлу словаря, где каждая строка содержит один
   * токен
   */
  SimpleTokenizer(const std::string &vocab_path);

  /**
   * @brief Кодирует текст в последовательность ID токенов
   *
   * @param text Входной текст для кодирования
   * @param max_len Максимальная длина выходной последовательности (с учетом
   * паддинга)
   * @return EncodingResult Полная информация о токенизации
   */
  EncodingResult encode(const std::string &text, size_t max_len = 128);

  /**
   * @brief Декодирует последовательность ID токенов обратно в текст
   *
   * @param ids Вектор ID токенов для декодирования
   * @return std::string Восстановленный текст
   */
  std::string decode(const std::vector<int64_t> &ids);

  /**
   * @brief Токенизирует текст с возвратом смещений (для интеграции с ONNX)
   *
   * @param text Входной текст для токенизации
   * @param max_len Максимальная длина выходной последовательности
   * @return TokenizationResult Результат токенизации
   */
  TokenizationResult tokenize_with_offsets(const std::string &text,
                                           size_t max_len = 128);

  /**
   * @brief Токенизирует текст без дополнительной информации
   *
   * @param text Входной текст для токенизации
   * @return std::vector<std::string> Вектор строковых представлений токенов
   */
  std::vector<std::string> tokenize_text(const std::string &text);

  /**
   * @brief Выводит информацию о словаре в консоль
   *
   * Отображает размер словаря, наличие специальных токенов
   * и проверяет несколько тестовых русских слов.
   */
  void print_vocab_info() const;

  /**
   * @brief Получает ID токена [CLS]
   *
   * @return int64_t ID токена [CLS] (обычно 101 для BERT)
   */
  inline int64_t get_cls_token_id() const { return 101; }

  /**
   * @brief Получает ID токена [SEP]
   *
   * @return int64_t ID токена [SEP] (обычно 102 для BERT)
   */
  inline int64_t get_sep_token_id() const { return 102; }

  /**
   * @brief Получает ID токена [PAD]
   *
   * @return int64_t ID токена [PAD] (обычно 0 для BERT)
   */
  inline int64_t get_pad_token_id() const { return 0; }

  /**
   * @brief Получает ID токена [UNK]
   *
   * @return int64_t ID токена [UNK] (обычно 100 для BERT)
   */
  inline int64_t get_unk_token_id() const { return 100; }

  /**
   * @brief Получает строковое представление токена [CLS]
   *
   * @return std::string Строка "[CLS]"
   */
  inline std::string get_cls_token() const { return "[CLS]"; }

  /**
   * @brief Получает строковое представление токена [SEP]
   *
   * @return std::string Строка "[SEP]"
   */
  inline std::string get_sep_token() const { return "[SEP]"; }

  /**
   * @brief Получает строковое представление токена [PAD]
   *
   * @return std::string Строка "[PAD]"
   */
  inline std::string get_pad_token() const { return "[PAD]"; }

  /**
   * @brief Получает строковое представление токена [UNK]
   *
   * @return std::string Строка "[UNK]"
   */
  inline std::string get_unk_token() const { return "[UNK]"; }

private:
  std::vector<std::string> vocabulary_; // Список всех токенов в словаре
  std::unordered_map<std::string, int64_t>
      vocab_map_; // Отображение токен -> ID
  std::unordered_map<int64_t, std::string>
      id_to_token_; // Отображение ID -> токен

  /**
   * @brief Разбивает текст на токены с использованием WordPiece алгоритма
   *
   * @param text Входной текст
   * @return std::vector<std::string> Вектор токенов
   */
  std::vector<std::string> split_text_into_tokens(const std::string &text);

  /**
   * @brief Находит ID токена в словаре
   *
   * @param token Строковое представление токена
   * @return int64_t ID токена или ID [UNK], если токен не найден
   */
  int64_t find_token_in_vocab(const std::string &token);

  /**
   * @brief Находит позицию токена в исходном тексте
   *
   * @param text Исходный текст
   * @param token Токен для поиска
   * @param start_pos Начальная позиция поиска (в байтах)
   * @return std::pair<size_t, size_t> Пара байтовых позиций (начало, конец)
   */
  std::pair<size_t, size_t> find_token_in_text(const std::string &text,
                                               const std::string &token,
                                               size_t start_pos);
};

#endif
