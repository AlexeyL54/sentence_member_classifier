#include "text_splitter.hpp"
#include "unistring.hpp"
#include <cctype>
#include <cstddef>
#include <unicode/brkiter.h>

#include <QRegularExpression>
#include <QString>

namespace utf8 {

/**
 * @brief Проверяет, является ли символ пробельным.
 * @param ch Символ для проверки.
 * @return true, если символ пробельный.
 */
bool TextSplitter::isSpace(const Unistring &ch) {
  std::string s = ch.to_string();
  if (s.empty())
    return true;

  // Проверка на ASCII пробельные символы
  if (s.size() == 1 && std::isspace(static_cast<unsigned char>(s[0]))) {
    return true;
  }

  return false;
}

/**
 * @brief Проверяет, является ли символ пунктуацией.
 * @param ch Символ для проверки.
 * @return true, если символ является пунктуацией.
 */
bool TextSplitter::isPunctuation(const Unistring &ch) {
  std::string s = ch.to_string();
  if (s.empty())
    return false;

  // Стандартные знаки препинания ASCII
  static const std::string ascii_punct = ",.!?;:()-*";
  if (s.size() == 1 && ascii_punct.find(s[0]) != std::string::npos) {
    return true;
  }

  // Русская тире (U+2014) и похожие символы
  if (s == "—" || s == "–" || s == "-")
    return true;

  // Кавычки-елочки
  if (s == "«" || s == "»")
    return true;

  // Многоточие как единый символ (если встречается)
  if (s == "…")
    return true;

  // Многоточие из трех точек
  if (s == "...")
    return true;

  if (s == "“" || s == "„")
    return true;

  if (s == "\"")
    return true;

  return false;
}

/**
 * @brief Проверяет, является ли символ буквой или цифрой.
 * @param ch Символ для проверки.
 * @return true, если символ буква или цифра.
 */
bool TextSplitter::isLetterOrDigit(const Unistring &ch) {
  std::string s = ch.to_string();
  if (s.empty())
    return false;

  // ASCII буквы и цифры
  if (s.size() == 1) {
    unsigned char c = static_cast<unsigned char>(s[0]);
    if (std::isalnum(c))
      return true;
  }

  // Многобайтовые символы (кириллица и др.) считаем буквами
  // Если это не пробел и не пунктуация, то это часть слова
  if (!isSpace(ch) && !isPunctuation(ch)) {
    return true;
  }

  return false;
}

/**
 * @brief Очищает слово от пунктуации.
 * @param word Слово для очистки.
 * @return Очищенное слово.
 */
Unistring TextSplitter::cleanWord(const Unistring &word) {
  Unistring result;

  for (size_t i = 0; i < word.length(); ++i) {
    Unistring ch = word[i];
    // Удаляем только явную пунктуацию, но оставляем буквы (включая многоточие
    // внутри слова)
    if (!isPunctuation(ch)) {
      result += ch;
    }
  }

  return result;
}

/**
 * @brief Создаёт токен слова.
 * @param text Текст токена.
 * @return Токен слова.
 */
TextToken createWordToken(const Unistring &text) {
  TextToken token;
  token.text = text;
  token.is_word = true;
  token.is_space = false;
  return token;
}

/**
 * @brief Создаёт токен пробела.
 * @param ch Символ пробела.
 * @return Токен пробела.
 */
TextToken createSpaceToken(const Unistring &ch) {
  TextToken token;
  token.text = ch;
  token.is_word = false;
  token.is_space = true;
  return token;
}

/**
 * @brief Создаёт токен пунктуации.
 * @param ch Символ пунктуации.
 * @return Токен пунктуации.
 */
TextToken createPunctToken(const Unistring &ch) {
  TextToken token;
  token.text = ch;
  token.is_word = false;
  token.is_space = false;
  return token;
}

/**
 * @brief Проверяет, является ли дефис частью слова.
 * @param ch Символ для проверки.
 * @param in_word Флаг, находимся ли мы внутри слова.
 * @param current_word Текущее слово.
 * @param text Полный текст.
 * @param i Позиция символа в тексте.
 * @param isLetterOrDigit Функция проверки на букву/цифру.
 * @return true, если дефис является частью слова.
 */
bool isHyphenInWord(const Unistring &ch, bool in_word,
                    const Unistring &current_word, const Unistring &text,
                    size_t i, bool (*isLetterOrDigit)(const Unistring &)) {
  bool is_hyphen_in_word = false;
  std::string ch_str = ch.to_string();
  if (ch_str == "-" || ch_str == "–" || ch_str == "—") {
    // Проверяем, есть ли буква перед текущей позицией (в текущем слове)
    // и есть ли буква после текущей позиции
    bool has_letter_before = in_word && current_word.length() > 0;
    bool has_letter_after = false;
    if (i + 1 < text.length()) {
      Unistring next_ch = text[i + 1];
      has_letter_after = isLetterOrDigit(next_ch);
    }
    is_hyphen_in_word = has_letter_before && has_letter_after;
  }
  return is_hyphen_in_word;
}

/**
 * @brief Обрабатывает многоточие в токенизаторе.
 * @param text Полный текст.
 * @param i Текущая позиция (ссылка для обновления).
 * @return Токен многоточия или пустой токен.
 */
TextToken handleEllipsisInTokenize(const Unistring &text, size_t &i) {
  TextToken empty_token;
  empty_token.text = Unistring();

  // Смотрим вперед на две точки
  if (i + 2 < text.length() && text[i + 1].to_string() == "." &&
      text[i + 2].to_string() == ".") {
    // Это начало многоточия
    Unistring ellipsis("...");
    i += 2; // Пропускаем следующие две точки

    TextToken punct_token;
    punct_token.text = ellipsis;
    punct_token.is_word = false;
    punct_token.is_space = false;
    return punct_token;
  }
  return empty_token;
}

/**
 * @brief Токенизирует текст на слова, пробелы и знаки препинания.
 * @param text Текст для токенизации.
 * @return Вектор токенов.
 */
std::vector<TextToken> TextSplitter::tokenize(const Unistring &text) {
  std::vector<TextToken> tokens;

  if (text.length() == 0) {
    return tokens;
  }

  Unistring current_word;
  bool in_word = false;

  for (size_t i = 0; i < text.length(); ++i) {
    Unistring ch = text[i];

    bool is_space = isSpace(ch);
    bool is_punct = isPunctuation(ch);
    bool is_letter = isLetterOrDigit(ch);

    if (is_space) {
      // Завершаем текущее слово если оно есть
      if (in_word) {
        tokens.push_back(createWordToken(current_word));
        current_word = Unistring();
        in_word = false;
      }

      // Добавляем пробел как отдельный токен
      tokens.push_back(createSpaceToken(ch));

    } else if (is_punct) {
      // Специальная обработка дефиса внутри слова (например, "что-то")
      bool is_hyphen =
          isHyphenInWord(ch, in_word, current_word, text, i, isLetterOrDigit);

      if (is_hyphen) {
        // Дефис внутри слова - добавляем его к текущему слову
        if (!in_word) {
          in_word = true;
          current_word = ch;
        } else {
          current_word += ch;
        }
        continue;
      }

      // Завершаем текущее слово если оно есть
      if (in_word) {
        tokens.push_back(createWordToken(current_word));
        current_word = Unistring();
        in_word = false;
      }

      // Проверяем на многоточие (три точки подряд)
      if (ch.to_string() == ".") {
        TextToken ellipsis_token = handleEllipsisInTokenize(text, i);
        if (ellipsis_token.text.length() > 0) {
          tokens.push_back(ellipsis_token);
          continue;
        }
      }

      // Добавляем знак препинания как отдельный токен
      tokens.push_back(createPunctToken(ch));

    } else if (is_letter) {
      // Начинаем или продолжаем слово
      if (!in_word) {
        in_word = true;
        current_word = ch;
      } else {
        current_word += ch;
      }
    }
  }

  // Добавляем последнее слово если оно есть
  if (in_word) {
    tokens.push_back(createWordToken(current_word));
  }

  return tokens;
}

std::vector<Unistring> TextSplitter::splitIntoSentences(const Unistring &text) {
  std::vector<Unistring> sentences;
  QString qstr = QString::fromStdString(text.to_string());

  if (qstr.trimmed().isEmpty()) {
    return sentences;
  }

  QRegularExpression sentenceRegex("("
                                   "[^.!?…]*"
                                   "(?:"
                                   "[.!?]"
                                   "(?:"
                                   "[.]*"
                                   "|…"
                                   ")?"
                                   "[»\"]*"
                                   "|"
                                   "…"
                                   "|"
                                   "[^.!?…]+»"
                                   ")"
                                   ")\\s*");

  QRegularExpressionMatchIterator iterator = sentenceRegex.globalMatch(qstr);

  int lastPos = 0;
  while (iterator.hasNext()) {
    QRegularExpressionMatch match = iterator.next();
    QString sentence = match.captured(1).trimmed();

    if (!sentence.isEmpty()) {
      sentences.push_back(Unistring(sentence.toStdString()));
      lastPos = match.capturedEnd();
    }
  }

  // Добавляем остаток текста после последнего найденного предложения
  if (lastPos < qstr.length()) {
    QString remaining = qstr.mid(lastPos).trimmed();
    if (!remaining.isEmpty()) {
      sentences.push_back(Unistring(remaining.toStdString()));
    }
  }

  // Если совсем ничего не нашли, добавляем весь текст как одно предложение
  if (sentences.empty() && !qstr.trimmed().isEmpty()) {
    sentences.push_back(text);
  }

  return sentences;
}
} // namespace utf8
