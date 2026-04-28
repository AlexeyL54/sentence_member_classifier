#include "text_splitter.hpp"
#include <cctype>

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
  static const std::string ascii_punct = ",.!?;:()-";
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

/**
 * @brief Извлекает подстроку предложения и обрезает пробелы.
 * @param text Полный текст.
 * @param start Начальная позиция.
 * @param end Конечная позиция.
 * @return Обрезанное предложение.
 */
Unistring extractSentence(const Unistring &text, size_t start, size_t end) {
  Unistring sent = text.substr(start, end - start);
  std::string sent_str = sent.to_string();

  // Обрезаем пробелы в конце предложения
  while (!sent_str.empty() &&
         std::isspace(static_cast<unsigned char>(sent_str.back()))) {
    sent_str.pop_back();
  }

  return Unistring(sent_str);
}

/**
 * @brief Пропускает пробелы после конца предложения.
 * @param text Полный текст.
 * @param start Начальная позиция для пропуска.
 * @return Позиция после пробелов.
 */
size_t skipSpacesAfterSentence(const Unistring &text, size_t start) {
  size_t char_count = text.length();
  size_t j = start;
  while (j < char_count) {
    Unistring j_ch = text[j];
    std::string j_str = j_ch.to_string();
    if (!j_str.empty() && !std::isspace(static_cast<unsigned char>(j_str[0]))) {
      break;
    }
    j++;
  }
  return j;
}

/**
 * @brief Разбивает текст на предложения.
 * @param text Текст для разбиения.
 * @return Вектор предложений.
 */
std::vector<Unistring> TextSplitter::splitIntoSentences(const Unistring &text) {
  std::vector<Unistring> sentences;

  if (text.length() == 0) {
    return sentences;
  }

  size_t char_count = text.length();
  size_t sentence_start = 0;

  for (size_t i = 0; i < char_count; ++i) {
    Unistring ch = text[i];
    std::string ch_str = ch.to_string();

    bool is_sentence_end =
        (ch_str == "." || ch_str == "!" || ch_str == "?" || ch_str == "\n");

    // Проверка на многоточие (...)
    bool is_ellipsis = false;
    int ellipsis_skip = 0;
    if (ch_str == ".") {
      if (i + 2 < char_count) {
        Unistring next1 = text[i + 1];
        Unistring next2 = text[i + 2];

        if (next1.to_string() == "." && next2.to_string() == ".") {
          is_ellipsis = true;
          is_sentence_end = true;
          ellipsis_skip = 2; // Пропустим следующие 2 точки
        }
      }
    }

    if (is_sentence_end) {
      Unistring sent;
      size_t next_start;

      if (is_ellipsis) {
        // Многоточие - включаем все три точки
        size_t sentence_end = i + 3;
        if (sentence_end > char_count) {
          sentence_end = char_count;
        }
        sent = extractSentence(text, sentence_start, sentence_end);
        next_start = skipSpacesAfterSentence(text, i + 1 + ellipsis_skip);
        i = next_start - 1;
      } else {
        // Обычный конец предложения
        sent = extractSentence(text, sentence_start, i + 1);
        next_start = skipSpacesAfterSentence(text, i + 1);
        i = next_start - 1;
      }

      if (!sent.to_string().empty()) {
        sentences.push_back(sent);
      }

      sentence_start = next_start;
    }
  }

  // Добавляем последнее предложение если оно не пустое
  if (sentence_start < char_count) {
    Unistring sent = extractSentence(text, sentence_start, char_count);
    if (!sent.to_string().empty()) {
      sentences.push_back(sent);
    }
  }

  return sentences;
}

/**
 * @brief Извлекает слова из вектора токенов.
 * @param tokens Вектор токенов.
 * @return Вектор словесных токенов.
 */
std::vector<TextToken>
TextSplitter::extractWords(const std::vector<TextToken> &tokens) {
  std::vector<TextToken> words;

  for (const auto &token : tokens) {
    if (token.is_word && !token.is_space) {
      words.push_back(token);
    }
  }

  return words;
}

} // namespace utf8
