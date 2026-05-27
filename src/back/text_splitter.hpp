#pragma once
#include "unistring.hpp"
#include <vector>

namespace utf8 {

/**
 * @brief Структура, описывающая токен (слово или знак препинания).
 */
struct TextToken {
  Unistring text;    // Текст токена
  size_t byte_start; // Начало в байтах в исходном тексте
  size_t byte_end;   // Конец в байтах в исходном тексте (включительно)
  bool is_word;      // true если это слово, false если пунктуация/пробел
  bool is_space;     // true если это пробельный символ
};

/**
 * @brief Класс для разбиения текста на токены с корректной обработкой UTF-8.
 */
class TextSplitter {
public:
  /**
   * @brief Разбивает текст на токены (слова и знаки препинания).
   * @param text Исходный текст.
   * @return Вектор токенов.
   */
  static std::vector<TextToken> tokenize(const Unistring &text);

  /**
   * @brief Разбивает текст на предложения.
   * @param text Исходный текст.
   * @return Вектор предложений.
   */
  static std::vector<Unistring> splitIntoSentences(const Unistring &text);

  /**
   * @brief Проверяет, является ли символ знаком препинания.
   * @param ch Символ для проверки.
   * @return true если это знак препинания.
   */
  static bool isPunctuation(const Unistring &ch);

  /**
   * @brief Проверяет, является ли символ пробельным.
   * @param ch Символ для проверки.
   * @return true если это пробельный символ.
   */
  static bool isSpace(const Unistring &ch);

  /**
   * @brief Проверяет, является ли символ буквой или цифрой.
   * @param ch Символ для проверки.
   * @return true если это буква или цифра.
   */
  static bool isLetterOrDigit(const Unistring &ch);

  /**
   * @brief Очищает слово от встроенных знаков препинания (опечатки).
   * @param word Слово для очистки.
   * @return Очищенное слово.
   */
  static Unistring cleanWord(const Unistring &word);
};

} // namespace utf8
