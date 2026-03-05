#include "simple_tokenizer.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace std;

/**
 * @brief Конструктор класса SimpleTokenizer
 *
 * Загружает словарь из файла и инициализирует внутренние структуры данных
 * для быстрого доступа к токенам по индексу и строке.
 *
 * @param vocab_path Путь к файлу словаря, где каждая строка содержит один токен
 * @throws runtime_error Если не удается открыть файл словаря
 */
SimpleTokenizer::SimpleTokenizer(const string &vocab_path) {
  // Загрузка словаря из файла
  ifstream vocab_file(vocab_path);
  if (!vocab_file.is_open()) {
    throw runtime_error("Cannot open vocabulary file: " + vocab_path);
  }

  string token;
  int64_t index = 0;

  while (getline(vocab_file, token)) {
    // Убираем символы новой строки и возврата каретки
    token.erase(remove(token.begin(), token.end(), '\r'), token.end());
    token.erase(remove(token.begin(), token.end(), '\n'), token.end());

    if (!token.empty()) {
      vocabulary_.push_back(token);
      vocab_map_[token] = index;
      id_to_token_[index] = token;
      index++;
    }
  }

  cout << "Tokenizer loaded. Vocabulary size: " << vocabulary_.size() << endl;
}

/**
 * @brief Проверяет, является ли байт началом UTF-8 символа кириллицы
 *
 * @param c Байт для проверки
 * @return true Если байт является началом кириллического символа в UTF-8
 * @return false В противном случае
 */
bool is_utf8_cyrillic_start(unsigned char c) {
  return (c >= 0xD0 &&
          c <= 0xD1); // Байты, с которых начинаются русские буквы в UTF-8
}

/**
 * @brief Проверяет, является ли строка русской буквой в UTF-8
 *
 * @param c Строка для проверки (ожидается 2 байта для кириллицы)
 * @return true Если строка является кириллическим символом
 * @return false В противном случае
 */
bool is_cyrillic_char(const string &c) {
  if (c.length() == 2) {
    unsigned char c1 = static_cast<unsigned char>(c[0]);
    unsigned char c2 = static_cast<unsigned char>(c[1]);

    // Диапазоны русских букв в UTF-8
    // А-Я: 0xD0 0x90 - 0xD0 0xAF
    // а-п: 0xD0 0xB0 - 0xD0 0xBF
    // р-я: 0xD1 0x80 - 0xD1 0x8F
    // ё: 0xD1 0x91
    // Ё: 0xD0 0x81

    if (c1 == 0xD0) {
      return (c2 >= 0x90 && c2 <= 0xBF) || c2 == 0x81;
    } else if (c1 == 0xD1) {
      return (c2 >= 0x80 && c2 <= 0x8F) || c2 == 0x91;
    }
  }
  return false;
}

/**
 * @brief Преобразует русскую букву в нижний регистр
 *
 * @param c Строка с кириллическим символом
 * @return string Символ в нижнем регистре или исходная строка, если
 * преобразование невозможно
 */
string to_lower_cyrillic(const string &c) {
  if (c.length() != 2)
    return c;

  unsigned char c1 = static_cast<unsigned char>(c[0]);
  unsigned char c2 = static_cast<unsigned char>(c[1]);

  string result;
  result.resize(2);

  if (c1 == 0xD0) {
    if (c2 >= 0x90 && c2 <= 0x9F) {
      // А-П -> а-п
      result[0] = 0xD0;
      result[1] = c2 + 0x20;
      return result;
    } else if (c2 >= 0xA0 && c2 <= 0xAF) {
      // Р-Я -> р-я (в D1 диапазоне)
      result[0] = 0xD1;
      result[1] = c2 - 0x20;
      return result;
    } else if (c2 == 0x81) {
      // Ё -> ё
      result[0] = 0xD1;
      result[1] = 0x91;
      return result;
    }
  }

  return c;
}

/**
 * @brief Проверяет, является ли строка пробельным символом
 *
 * @param s Строка для проверки
 * @return true Если строка является пробельным символом
 * @return false В противном случае
 */
bool is_whitespace_utf8(const string &s) {
  if (s.length() == 1) {
    char c = s[0];
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
           c == '\v';
  }
  return false;
}

/**
 * @brief Проверяет, является ли строка знаком пунктуации
 *
 * @param s Строка для проверки
 * @return true Если строка является знаком пунктуации
 * @return false В противном случае
 */
bool is_punctuation_utf8(const string &s) {
  if (s.length() == 1) {
    char c = s[0];
    // ASCII пунктуация
    return ispunct(static_cast<unsigned char>(c));
  } else if (s.length() == 2) {
    // Русская пунктуация и тире
    unsigned char c1 = static_cast<unsigned char>(s[0]);
    unsigned char c2 = static_cast<unsigned char>(s[1]);

    // Кавычки, тире и другие русские знаки
    if (c1 == 0xE2 && c2 == 0x80) {
      // Это может быть длинное тире, кавычки и т.д.
      return true;
    }

    // Проверяем известные знаки пунктуации в русском
    vector<pair<unsigned char, unsigned char>> punct = {
        {0xD0, 0x81}, // Ё (но это буква, не пунктуация)
        {0xE2, 0x80},
        {0xE2, 0x82} // Разные символы
    };

    // Упрощенно: считаем все, что не буква и не пробел, пунктуацией
    return !is_cyrillic_char(s) && !is_whitespace_utf8(s);
  }
  return true; // По умолчанию считаем пунктуацией
}

/**
 * @brief Разбивает UTF-8 строку на отдельные символы
 *
 * @param s Входная UTF-8 строка
 * @return vector<string> Вектор строк, каждая из которых содержит один UTF-8
 * символ
 */
vector<string> utf8_split(const string &s) {
  vector<string> chars;
  for (size_t i = 0; i < s.length();) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    size_t len = 1;

    // Определяем длину UTF-8 символа
    if (c >= 0xF0)
      len = 4;
    else if (c >= 0xE0)
      len = 3;
    else if (c >= 0xC0)
      len = 2;

    if (i + len <= s.length()) {
      chars.push_back(s.substr(i, len));
    } else {
      chars.push_back(s.substr(i, 1));
    }
    i += len;
  }
  return chars;
}

/**
 * @brief Разбивает текст на токены с использованием WordPiece алгоритма
 *
 * @param text Входной текст для токенизации
 * @return vector<string> Вектор токенов, полученных в результате WordPiece
 * токенизации
 */
vector<string> SimpleTokenizer::split_text_into_tokens(const string &text) {
  vector<string> tokens;
  vector<string> words;
  string current_word;

  // Разбиваем текст на отдельные символы UTF-8
  vector<string> chars = utf8_split(text);

  // Базовая токенизация на слова
  for (size_t i = 0; i < chars.size(); i++) {
    const string &c = chars[i];

    // Проверяем пробелы
    if (is_whitespace_utf8(c)) {
      if (!current_word.empty()) {
        // Приводим слово к нижнему регистру
        words.push_back(current_word);
        current_word.clear();
      }
      continue;
    }

    // Проверяем пунктуацию
    if (is_punctuation_utf8(c)) {
      if (!current_word.empty()) {
        words.push_back(current_word);
        current_word.clear();
      }
      words.push_back(c); // Добавляем пунктуацию как отдельное слово
      continue;
    }

    // Обычный символ (буква)
    current_word += c;
  }

  // Добавляем последнее слово
  if (!current_word.empty()) {
    words.push_back(current_word);
  }

  // Приводим слова к нижнему регистру (для поиска в словаре)
  for (size_t i = 0; i < words.size(); i++) {
    string &word = words[i];
    string lower_word;

    vector<string> word_chars = utf8_split(word);
    for (const string &c : word_chars) {
      if (is_cyrillic_char(c)) {
        lower_word += to_lower_cyrillic(c);
      } else {
        // Для латиницы используем стандартный tolower
        for (char ch : c) {
          lower_word += tolower(static_cast<unsigned char>(ch));
        }
      }
    }

    if (!lower_word.empty()) {
      word = lower_word;
    }
  }

  // WordPiece токенизация для каждого слова
  for (const string &word : words) {
    // Пропускаем пунктуацию (она должна быть в словаре)
    if (word.length() == 1 && ispunct(static_cast<unsigned char>(word[0]))) {
      auto it = vocab_map_.find(word);
      if (it != vocab_map_.end()) {
        tokens.push_back(word);
      } else {
        tokens.push_back(get_unk_token());
      }
      continue;
    }

    // Для слишком длинных слов - сразу UNK
    if (word.length() > 100) {
      tokens.push_back(get_unk_token());
      continue;
    }

    // WordPiece разбиение
    vector<string> wordpieces;
    int start = 0;
    bool is_bad = false;

    while (start < word.length()) {
      int end = word.length();
      string cur_substr;
      bool found = false;

      // Ищем самое длинное подслово
      while (end > start) {
        string substr = word.substr(start, end - start);

        // Для не-начала слова добавляем ##
        if (start > 0) {
          substr = "##" + substr;
        }

        auto it = vocab_map_.find(substr);
        if (it != vocab_map_.end()) {
          cur_substr = substr;
          found = true;
          break;
        }
        end--;
      }

      if (!found) {
        is_bad = true;
        break;
      }

      wordpieces.push_back(cur_substr);
      start = end;
    }

    if (is_bad) {
      tokens.push_back(get_unk_token());
    } else {
      tokens.insert(tokens.end(), wordpieces.begin(), wordpieces.end());
    }
  }

  return tokens;
}

/**
 * @brief Находит позицию токена в исходном тексте
 *
 * @param text Исходный текст
 * @param token Токен для поиска
 * @param start_pos Начальная позиция поиска (в байтах)
 * @return pair<size_t, size_t> Пара байтовых позиций начала и конца токена
 */
pair<size_t, size_t> SimpleTokenizer::find_token_in_text(const string &text,
                                                         const string &token,
                                                         size_t start_pos) {
  if (token.empty() || start_pos >= text.size()) {
    return {start_pos, start_pos};
  }

  // Убираем ## для поиска
  string search_token = token;
  if (token.substr(0, 2) == "##") {
    search_token = token.substr(2);
  }

  // Разбиваем текст на символы для правильного поиска позиций
  auto text_chars = utf8_split(text);
  auto token_chars = utf8_split(search_token);

  // Конвертируем start_pos в индекс символа
  size_t char_index = 0;
  size_t byte_pos = 0;
  while (byte_pos < start_pos && char_index < text_chars.size()) {
    byte_pos += text_chars[char_index].length();
    char_index++;
  }

  // Ищем токен как последовательность символов
  for (size_t i = char_index; i + token_chars.size() <= text_chars.size();
       i++) {
    bool match = true;
    for (size_t j = 0; j < token_chars.size(); j++) {
      if (text_chars[i + j] != token_chars[j]) {
        match = false;
        break;
      }
    }

    if (match) {
      // Вычисляем байтовые позиции
      size_t start_byte = 0;
      for (size_t k = 0; k < i; k++) {
        start_byte += text_chars[k].length();
      }

      size_t end_byte = start_byte;
      for (size_t k = 0; k < token_chars.size(); k++) {
        end_byte += token_chars[k].length();
      }

      return {start_byte, end_byte};
    }
  }

  return {start_pos, start_pos + search_token.length()};
}

/**
 * @brief Находит ID токена в словаре
 *
 * @param token Строковое представление токена
 * @return int64_t ID токена в словаре или ID неизвестного токена, если токен не
 * найден
 */
int64_t SimpleTokenizer::find_token_in_vocab(const string &token) {
  // Прямой поиск
  auto it = vocab_map_.find(token);
  if (it != vocab_map_.end()) {
    return it->second;
  }

  // Поиск в нижнем регистре
  string lower_token = token;
  transform(lower_token.begin(), lower_token.end(), lower_token.begin(),
            [](unsigned char c) { return tolower(c); });

  it = vocab_map_.find(lower_token);
  if (it != vocab_map_.end()) {
    return it->second;
  }

  // Если не нашли, пробуем найти части слова
  // Сначала ищем начало слова
  for (size_t len = token.size(); len > 0; len--) {
    string prefix = token.substr(0, len);
    it = vocab_map_.find(prefix);
    if (it != vocab_map_.end()) {
      // Нашли префикс в словаре
      return it->second;
    }
  }

  // Не нашли - возвращаем UNK
  return get_unk_token_id();
}

/**
 * @brief Основной метод кодирования текста в последовательность ID токенов
 *
 * @param text Входной текст для кодирования
 * @param max_len Максимальная длина выходной последовательности (с учетом
 * паддинга)
 * @return EncodingResult Структура, содержащая входные ID, маску внимания,
 * токены и смещения
 */
SimpleTokenizer::EncodingResult SimpleTokenizer::encode(const string &text,
                                                        size_t max_len) {
  EncodingResult result;

  // Разделяем текст на токены
  auto raw_tokens = split_text_into_tokens(text);

  // Начинаем с [CLS]
  result.input_ids.push_back(get_cls_token_id());
  result.tokens.push_back(get_cls_token());
  result.offsets.push_back({0, 0});
  result.word_ids.push_back(-1);

  size_t current_pos = 0;
  int word_id = 0;

  for (const auto &raw_token : raw_tokens) {
    if (result.input_ids.size() >= max_len - 1)
      break; // -1 для [SEP]

    // Ищем токен в словаре
    int64_t token_id = find_token_in_vocab(raw_token);
    string actual_token;

    // Находим фактический токен из словаря
    auto it = id_to_token_.find(token_id);
    if (it != id_to_token_.end()) {
      actual_token = it->second;
    } else {
      actual_token = get_unk_token();
    }

    // Ищем позицию в тексте
    auto [start, end] = find_token_in_text(text, raw_token, current_pos);

    result.input_ids.push_back(token_id);
    result.tokens.push_back(actual_token);
    result.offsets.push_back({start, end});
    result.word_ids.push_back(word_id);

    word_id++;
    current_pos = end;

    // Пропускаем пробелы
    while (current_pos < text.size() && isspace(text[current_pos])) {
      current_pos++;
    }
  }

  // Добавляем [SEP]
  if (result.input_ids.size() < max_len) {
    result.input_ids.push_back(get_sep_token_id());
    result.tokens.push_back(get_sep_token());
    result.offsets.push_back({current_pos, current_pos});
    result.word_ids.push_back(-1);
  }

  // Паддинг
  while (result.input_ids.size() < max_len) {
    result.input_ids.push_back(get_pad_token_id());
    result.tokens.push_back(get_pad_token());
    result.offsets.push_back({0, 0});
    result.word_ids.push_back(-1);
  }

  // Создаем маску внимания
  for (size_t i = 0; i < max_len; i++) {
    result.attention_mask.push_back(
        result.input_ids[i] != get_pad_token_id() ? 1 : 0);
  }

  return result;
}

/**
 * @brief Декодирует последовательность ID токенов обратно в текст
 *
 * @param ids Вектор ID токенов для декодирования
 * @return string Восстановленный текст
 */
string SimpleTokenizer::decode(const vector<int64_t> &ids) {
  string text;

  for (size_t i = 0; i < ids.size(); i++) {
    int64_t id = ids[i];

    // Пропускаем специальные токены
    if (id == get_cls_token_id() || id == get_sep_token_id() ||
        id == get_pad_token_id()) {
      continue;
    }

    auto it = id_to_token_.find(id);
    if (it != id_to_token_.end()) {
      string token = it->second;

      // Для субтокенов (начинающихся с ##) не добавляем пробел
      if (token.substr(0, 2) == "##") {
        text += token.substr(2);
      } else {
        // Добавляем пробел перед новым словом
        if (!text.empty() && text.back() != ' ') {
          text += " ";
        }
        text += token;
      }
    } else if (id == get_unk_token_id()) {
      // Обработка UNK токена
      if (!text.empty() && text.back() != ' ') {
        text += " ";
      }
      text += "[UNK]";
    }
  }

  return text;
}

/**
 * @brief Токенизация текста с возвратом смещений в исходном тексте
 *
 * @param text Входной текст для токенизации
 * @param max_len Максимальная длина выходной последовательности
 * @return TokenizationResult Структура с ID токенов, маской внимания и
 * смещениями
 */
SimpleTokenizer::TokenizationResult
SimpleTokenizer::tokenize_with_offsets(const string &text, size_t max_len) {

  TokenizationResult result;
  auto encoding = encode(text, max_len);

  result.input_ids = encoding.input_ids;
  result.attention_mask = encoding.attention_mask;
  result.tokens = encoding.tokens;
  result.offsets = encoding.offsets;

  return result;
}

/**
 * @brief Токенизация текста без дополнительной информации
 *
 * @param text Входной текст для токенизации
 * @return vector<string> Вектор строковых представлений токенов
 */
vector<string> SimpleTokenizer::tokenize_text(const string &text) {
  auto encoding = encode(text, 512);
  return encoding.tokens;
}

/**
 * @brief Выводит информацию о словаре в консоль
 *
 * Отображает размер словаря, наличие специальных токенов
 * и проверяет наличие нескольких тестовых русских слов.
 */
void SimpleTokenizer::print_vocab_info() const {
  cout << "Vocabulary Info:" << endl;
  cout << "  Size: " << vocabulary_.size() << " tokens" << endl;

  // Проверяем специальные токены
  auto check_token = [this](const string &name, const string &token,
                            int64_t expected_id) {
    auto it = vocab_map_.find(token);
    if (it != vocab_map_.end()) {
      cout << "  " << name << ": " << token << " (ID: " << it->second
           << ", expected: " << expected_id << ")"
           << (it->second == expected_id ? " ✓" : " ✗") << endl;
    } else {
      cout << "  " << name << ": NOT FOUND (expected ID: " << expected_id << ")"
           << endl;
    }
  };

  check_token("[CLS]", get_cls_token(), get_cls_token_id());
  check_token("[SEP]", get_sep_token(), get_sep_token_id());
  check_token("[PAD]", get_pad_token(), get_pad_token_id());
  check_token("[UNK]", get_unk_token(), get_unk_token_id());

  // Ищем несколько русских слов в словаре
  vector<string> test_words = {"Вчера",   "я",     "ходил", "в",
                               "кино",    "Он",    "очень", "осторожно",
                               "перешел", "дорогу"};

  cout << "\nChecking Russian words in vocabulary:" << endl;
  for (const auto &word : test_words) {
    auto it = vocab_map_.find(word);
    if (it != vocab_map_.end()) {
      cout << "  " << word << ": FOUND (ID: " << it->second << ")" << endl;
    } else {
      cout << "  " << word << ": NOT FOUND" << endl;
    }
  }
}
