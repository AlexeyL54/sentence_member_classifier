#include "text_splitter.hpp"
#include <cctype>

namespace utf8 {

bool TextSplitter::isSpace(const Unistring &ch) {
  std::string s = ch.to_string();
  if (s.empty())
    return true;

  // Проверка на ASCII пробельные символы
  if (s.size() == 1 && std::isspace(static_cast<unsigned char>(s[0]))) {
    return true;
  }

  // Проверка на многоточие (три точки)
  if (s == "...")
    return true;

  return false;
}

bool TextSplitter::isPunctuation(const Unistring &ch) {
  std::string s = ch.to_string();
  if (s.empty())
    return false;

  // Стандартные знаки препинания ASCII
  static const std::string ascii_punct = ",.!?;:()-";
  if (s.size() == 1 && ascii_punct.find(s[0]) != std::string::npos) {
    return true;
  }

  // Русская тире (U+2014)
  if (s == "—")
    return true;

  // Кавычки-елочки
  if (s == "«" || s == "»")
    return true;

  // Многоточие как единый символ (если встречается)
  if (s == "…")
    return true;

  return false;
}

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

std::vector<TextToken> TextSplitter::tokenize(const Unistring &text) {
  std::vector<TextToken> tokens;

  if (text.length() == 0) {
    return tokens;
  }

  // Получаем смещения байтов для корректного отслеживания позиций
  std::vector<size_t> byte_offsets = text.get_char_offsets();
  std::string text_str = text.to_string();

  Unistring current_word;
  size_t word_start_byte = 0;
  bool in_word = false;

  for (size_t i = 0; i < text.length(); ++i) {
    Unistring ch = text[i];
    size_t ch_byte_start = byte_offsets[i];
    size_t ch_byte_end = (i + 1 < byte_offsets.size()) ? byte_offsets[i + 1] - 1
                                                       : text_str.length() - 1;

    bool is_space = isSpace(ch);
    bool is_punct = isPunctuation(ch);
    bool is_letter = isLetterOrDigit(ch);

    if (is_space) {
      // Завершаем текущее слово если оно есть
      if (in_word) {
        TextToken token;
        token.text = current_word;
        token.byte_start = word_start_byte;
        token.byte_end = byte_offsets[i] - 1;
        token.is_word = true;
        token.is_space = false;
        tokens.push_back(token);
        current_word = Unistring();
        in_word = false;
      }

      // Добавляем пробел как отдельный токен
      TextToken space_token;
      space_token.text = ch;
      space_token.byte_start = ch_byte_start;
      space_token.byte_end = ch_byte_end;
      space_token.is_word = false;
      space_token.is_space = true;
      tokens.push_back(space_token);

    } else if (is_punct) {
      // Завершаем текущее слово если оно есть
      if (in_word) {
        TextToken token;
        token.text = current_word;
        token.byte_start = word_start_byte;
        token.byte_end = byte_offsets[i] - 1;
        token.is_word = true;
        token.is_space = false;
        tokens.push_back(token);
        current_word = Unistring();
        in_word = false;
      }

      // Проверяем на многоточие (три точки подряд)
      if (ch.to_string() == ".") {
        // Смотрим вперед на две точки
        if (i + 2 < text.length() && text[i + 1].to_string() == "." &&
            text[i + 2].to_string() == ".") {
          // Это начало многоточия
          Unistring ellipsis("...");
          size_t ellipsis_start = ch_byte_start;
          size_t ellipsis_end = byte_offsets[i + 2 + 1 > byte_offsets.size()
                                                 ? byte_offsets.size() - 1
                                                 : i + 3] -
                                1;

          TextToken punct_token;
          punct_token.text = ellipsis;
          punct_token.byte_start = ellipsis_start;
          punct_token.byte_end = ellipsis_end;
          punct_token.is_word = false;
          punct_token.is_space = false;
          tokens.push_back(punct_token);

          i += 2; // Пропускаем следующие две точки
          continue;
        }
      }

      // Добавляем знак препинания как отдельный токен
      TextToken punct_token;
      punct_token.text = ch;
      punct_token.byte_start = ch_byte_start;
      punct_token.byte_end = ch_byte_end;
      punct_token.is_word = false;
      punct_token.is_space = false;
      tokens.push_back(punct_token);

    } else if (is_letter) {
      // Начинаем или продолжаем слово
      if (!in_word) {
        in_word = true;
        word_start_byte = ch_byte_start;
        current_word = ch;
      } else {
        current_word += ch;
      }
    }
  }

  // Добавляем последнее слово если оно есть
  if (in_word) {
    TextToken token;
    token.text = current_word;
    token.byte_start = word_start_byte;
    token.byte_end = text_str.length() - 1;
    token.is_word = true;
    token.is_space = false;
    tokens.push_back(token);
  }

  return tokens;
}

std::vector<Unistring> TextSplitter::splitIntoSentences(const Unistring &text) {
  std::vector<Unistring> sentences;

  if (text.length() == 0) {
    return sentences;
  }

  std::string text_str = text.to_string();
  std::vector<size_t> byte_offsets = text.get_char_offsets();
  size_t char_count = text.length();

  Unistring current_sentence;
  size_t sentence_start_byte = 0;

  for (size_t i = 0; i < char_count; ++i) {
    // Получаем символ напрямую из строки через смещение
    size_t byte_start = byte_offsets[i];
    size_t byte_end =
        (i + 1 < byte_offsets.size()) ? byte_offsets[i + 1] : text_str.length();
    std::string ch_str = text_str.substr(byte_start, byte_end - byte_start);

    bool is_sentence_end =
        (ch_str == "." || ch_str == "!" || ch_str == "?" || ch_str == "\n");

    // Проверка на многоточие (...)
    if (!is_sentence_end && ch_str == ".") {
      if (i + 2 < char_count) {
        size_t next1_start = byte_offsets[i + 1];
        size_t next1_end = (i + 2 < byte_offsets.size()) ? byte_offsets[i + 2]
                                                         : text_str.length();
        size_t next2_start = byte_offsets[i + 2];
        size_t next2_end = (i + 3 < byte_offsets.size()) ? byte_offsets[i + 3]
                                                         : text_str.length();

        std::string next1_str =
            text_str.substr(next1_start, next1_end - next1_start);
        std::string next2_str =
            text_str.substr(next2_start, next2_end - next2_start);

        if (next1_str == "." && next2_str == ".") {
          is_sentence_end = true;
          i += 2; // Пропускаем следующие две точки
        }
      }
    }

    if (is_sentence_end) {
      // Добавляем текущий символ и возможные дополнительные точки к предложению
      current_sentence += ch_str;
      if (ch_str == "." && i >= 2) {
        // Проверяем, были ли добавлены точки в цикле выше
        // На самом деле мы уже увеличили i, так что просто берем подстроку
      }

      // Берем подстроку от начала предложения до текущего момента
      size_t sentence_end_byte = (i + 1 < byte_offsets.size())
                                     ? byte_offsets[i + 1]
                                     : text_str.length();
      std::string sent_str = text_str.substr(
          sentence_start_byte, sentence_end_byte - sentence_start_byte);

      // Обрезаем пробелы в конце предложения
      while (!sent_str.empty() &&
             std::isspace(static_cast<unsigned char>(sent_str.back()))) {
        sent_str.pop_back();
      }

      if (!sent_str.empty()) {
        sentences.push_back(Unistring(sent_str));
      }

      current_sentence = Unistring();

      // Пропускаем пробелы после знака завершения
      size_t j = i + 1;
      while (j < char_count) {
        size_t j_byte_start = byte_offsets[j];
        size_t j_byte_end = (j + 1 < byte_offsets.size()) ? byte_offsets[j + 1]
                                                          : text_str.length();
        std::string j_str =
            text_str.substr(j_byte_start, j_byte_end - j_byte_start);
        if (!std::isspace(static_cast<unsigned char>(j_str[0]))) {
          break;
        }
        j++;
      }

      sentence_start_byte =
          (j < byte_offsets.size()) ? byte_offsets[j] : text_str.length();
      i = j - 1;
    }
  }

  // Добавляем последнее предложение если оно не пустое
  // Это критично для случая, когда текст обрывается без точки
  if (sentence_start_byte < text_str.length()) {
    std::string sent_str = text_str.substr(sentence_start_byte);

    // Обрезаем пробелы в конце
    while (!sent_str.empty() &&
           std::isspace(static_cast<unsigned char>(sent_str.back()))) {
      sent_str.pop_back();
    }

    if (!sent_str.empty()) {
      sentences.push_back(Unistring(sent_str));
    }
  }

  return sentences;
}

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
