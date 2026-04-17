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

  // Русская тире (U+2014) и похожие символы
  if (s == "—" || s == "–" || s == "-")
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

  Unistring current_word;
  size_t word_start = 0;
  bool in_word = false;

  for (size_t i = 0; i < text.length(); ++i) {
    Unistring ch = text[i];

    bool is_space = isSpace(ch);
    bool is_punct = isPunctuation(ch);
    bool is_letter = isLetterOrDigit(ch);

    if (is_space) {
      // Завершаем текущее слово если оно есть
      if (in_word) {
        TextToken token;
        token.text = current_word;
        token.is_word = true;
        token.is_space = false;
        tokens.push_back(token);
        current_word = Unistring();
        in_word = false;
      }

      // Добавляем пробел как отдельный токен
      TextToken space_token;
      space_token.text = ch;
      space_token.is_word = false;
      space_token.is_space = true;
      tokens.push_back(space_token);

    } else if (is_punct) {
      // Специальная обработка дефиса внутри слова (например, "что-то")
      // Дефис считается частью слова, если он окружён буквами с обеих сторон
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

      if (is_hyphen_in_word) {
        // Дефис внутри слова - добавляем его к текущему слову
        if (!in_word) {
          in_word = true;
          word_start = i;
          current_word = ch;
        } else {
          current_word += ch;
        }
        continue;
      }

      // Завершаем текущее слово если оно есть
      if (in_word) {
        TextToken token;
        token.text = current_word;
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

          TextToken punct_token;
          punct_token.text = ellipsis;
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
      punct_token.is_word = false;
      punct_token.is_space = false;
      tokens.push_back(punct_token);

    } else if (is_letter) {
      // Начинаем или продолжаем слово
      if (!in_word) {
        in_word = true;
        word_start = i;
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
      // Если это многоточие, добавляем все три точки к предложению
      if (is_ellipsis) {
        // Получаем подстроку от начала предложения до конца третьей точки
        size_t sentence_end = i + 3; // Включаем три точки
        if (sentence_end > char_count) {
          sentence_end = char_count;
        }

        Unistring sent = text.substr(sentence_start, sentence_end - 1);
        std::string sent_str = sent.to_string();

        // Обрезаем пробелы в конце предложения
        while (!sent_str.empty() &&
               std::isspace(static_cast<unsigned char>(sent_str.back()))) {
          sent_str.pop_back();
        }

        if (!sent_str.empty()) {
          sentences.push_back(Unistring(sent_str));
        }

        // Пропускаем пробелы после знака завершения
        size_t j = i + 1 + ellipsis_skip;
        while (j < char_count) {
          Unistring j_ch = text[j];
          std::string j_str = j_ch.to_string();
          if (!j_str.empty() &&
              !std::isspace(static_cast<unsigned char>(j_str[0]))) {
            break;
          }
          j++;
        }

        sentence_start = j;
        i = j - 1;
        continue;
      }

      // Обычный конец предложения (не многоточие)
      // Получаем подстроку от начала предложения до текущего символа
      // включительно
      Unistring sent = text.substr(sentence_start, i);
      std::string sent_str = sent.to_string();

      // Обрезаем пробелы в конце предложения
      while (!sent_str.empty() &&
             std::isspace(static_cast<unsigned char>(sent_str.back()))) {
        sent_str.pop_back();
      }

      if (!sent_str.empty()) {
        sentences.push_back(Unistring(sent_str));
      }

      // Пропускаем пробелы после знака завершения
      size_t j = i + 1;
      while (j < char_count) {
        Unistring j_ch = text[j];
        std::string j_str = j_ch.to_string();
        if (!j_str.empty() &&
            !std::isspace(static_cast<unsigned char>(j_str[0]))) {
          break;
        }
        j++;
      }

      sentence_start = j;
      i = j - 1;
    }
  }

  // Добавляем последнее предложение если оно не пустое
  // Это критично для случая, когда текст обрывается без точки
  if (sentence_start < char_count) {
    Unistring sent = text.substr(sentence_start, char_count - 1);
    std::string sent_str = sent.to_string();

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
