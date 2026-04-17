// src/back/simple_tokenizer.cpp
#include "simple_tokenizer.hpp"
#include "text_splitter.hpp"
#include "unistring.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

SimpleTokenizer::SimpleTokenizer(const string &vocab_path) {
  ifstream vocab_file(vocab_path);
  if (!vocab_file.is_open()) {
    throw runtime_error("Cannot open vocabulary file: " + vocab_path);
  }

  string token;
  int64_t index = 0;
  while (getline(vocab_file, token)) {
    // Удаляем символы возврата каретки и перевода строки
    token.erase(remove(token.begin(), token.end(), '\r'), token.end());
    token.erase(remove(token.begin(), token.end(), '\n'), token.end());

    if (!token.empty()) {
      vocabulary_.push_back(token);
      vocab_map_[token] = index;
      id_to_token_[index] = token;
      index++;
    }
  }
}

// --- Вспомогательные функции для разбиения текста ---

/**
@brief Проверяет, является ли символ (в виде Unistring) разделителем.
Разделителем считается: пробельный символ ASCII или любой знак,
не являющийся буквой или цифрой.
@param uni_char Символ в виде Unistring.
@return true, если символ является разделителем.
*/
static bool is_separator_char(const utf8::Unistring &uni_char) {
  string s = uni_char.to_string();
  if (s.empty())
    return true;

  // Пробельные символы ASCII
  if (s.size() == 1 && std::isspace(static_cast<unsigned char>(s[0]))) {
    return true;
  }

  // Знаки препинания (не буквы и не цифры)
  if (s.size() == 1) {
    return !std::isalnum(static_cast<unsigned char>(s[0]));
  }

  // Многобайтовые символы (кириллица и др.) считаем частью слова
  return false;
}

/**
@brief Разбивает текст на список слов и отдельных знаков препинания.
@param text Входной текст.
@return vector<string> Вектор слов и знаков препинания.
*/
vector<string> SimpleTokenizer::extract_words_and_punct(const string &text) {
  vector<string> result;

  // Используем новый TextSplitter для корректной токенизации
  utf8::Unistring uni_text(text);
  std::vector<utf8::TextToken> tokens = utf8::TextSplitter::tokenize(uni_text);

  for (const auto &token : tokens) {
    // Пропускаем пробелы, но добавляем слова и пунктуацию
    if (!token.is_space) {
      result.push_back(token.text.to_string());
    }
  }

  return result;
}

/**
@brief Приводит слово к нижнему регистру с использованием Unistring.
@param word Входное слово.
@return string Слово в нижнем регистре.
*/
string SimpleTokenizer::normalize_word(const string &word) {
  if (word.empty())
    return word;
  return utf8::Unistring(word).to_lower().to_string();
}

/**
@brief Применяет алгоритм WordPiece к одному слову.
@param word Входное слово.
@return vector<string> Вектор подслов (субтокенов).
*/
vector<string> SimpleTokenizer::wordpiece_split(const string &word) {
  vector<string> tokens;

  // Обработка одиночного знака препинания
  if (word.length() == 1 &&
      !std::isalnum(static_cast<unsigned char>(word[0]))) {
    auto it = vocab_map_.find(word);
    tokens.push_back(it != vocab_map_.end() ? word : get_unk_token());
    return tokens;
  }

  // Слишком длинные слова заменяем на [UNK]
  if (word.length() > 100) {
    tokens.push_back(get_unk_token());
    return tokens;
  }

  int start = 0;
  bool is_bad = false;

  while (start < static_cast<int>(word.length())) {
    int end = word.length();
    string cur_substr;
    bool found = false;

    while (end > start) {
      string substr = word.substr(start, end - start);
      if (start > 0) {
        substr = "##" + substr;
      }

      if (vocab_map_.count(substr)) {
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

    tokens.push_back(cur_substr);
    start = end;
  }

  if (is_bad) {
    return {get_unk_token()};
  }

  return tokens;
}

/**
@brief Основной метод разбиения текста на токены.
Объединяет извлечение слов/знаков, нормализацию и WordPiece-разбиение.
@param text Входной текст.
@return vector<string> Вектор финальных токенов.
*/
vector<string> SimpleTokenizer::split_text_into_tokens(const string &text) {
  vector<string> final_tokens;
  vector<string> raw_units = extract_words_and_punct(text);

  for (const string &unit : raw_units) {
    // Эвристика: определяем, является ли юнит словом (а не знаком препинания)
    bool is_word = (unit.length() > 1); // Многобайтовые символы считаем буквами
    if (!is_word && unit.length() == 1) {
      is_word = std::isalnum(static_cast<unsigned char>(unit[0]));
    }

    // Нормализуем только слова (знаки препинания не меняем)
    string processed_unit = is_word ? normalize_word(unit) : unit;
    vector<string> subtokens = wordpiece_split(processed_unit);
    final_tokens.insert(final_tokens.end(), subtokens.begin(), subtokens.end());
  }

  return final_tokens;
}

// --- Методы поиска ---

/**
@brief Находит позицию токена в исходном тексте, начиная с заданной байтовой
позиции. Корректно работает с многобайтовыми символами благодаря Unistring.
@param text Исходный текст.
@param token Токен для поиска (возможно с префиксом ##).
@param start_pos Начальная позиция поиска в байтах.
@return pair<size_t, size_t> Байтовые смещения (начало, конец) найденного
токена. Если токен не найден, возвращает {start_pos, start_pos} (пустой
диапазон), чтобы не ломать поток.
*/
pair<size_t, size_t> SimpleTokenizer::find_token_in_text(const string &text,
                                                         const string &token,
                                                         size_t start_pos) {
  if (token.empty())
    return {start_pos, start_pos};

  string search_token = token;
  if (token.size() >= 2 && token.substr(0, 2) == "##") {
    search_token = token.substr(2);
  }

  utf8::Unistring uni_text(text);
  utf8::Unistring uni_search(search_token);
  utf8::Unistring uni_text_lower = uni_text.to_lower();
  utf8::Unistring uni_search_lower = uni_search.to_lower();

  size_t start_char_idx = 0;
  vector<size_t> offsets = uni_text.get_char_offsets();

  for (size_t i = 0; i < offsets.size(); ++i) {
    if (offsets[i] >= start_pos) {
      start_char_idx = i;
      break;
    }
    if (i == offsets.size() - 1) {
      start_char_idx = offsets.size();
    }
  }

  size_t found_char_idx = uni_text_lower.find(uni_search_lower, start_char_idx);

  if (found_char_idx == SIZE_MAX) {
    // ИСПРАВЛЕНИЕ: Вместо возврата пустого диапазона, пробуем найти токен
    // начиная с начала текста или пропускаем его
    cerr << "Warning: Token '" << token << "' not found at byte " << start_pos
         << ". Skipping." << endl;

    // Возвращаем специальную метку - длина 0, но позиция продвигается
    // Найдем следующий пробел или конец слова
    size_t skip_pos = start_pos;
    while (skip_pos < text.size() &&
           !std::isspace(static_cast<unsigned char>(text[skip_pos]))) {
      skip_pos++;
    }
    // Возвращаем диапазон до следующего пробела (пропускаем это слово)
    return {start_pos, skip_pos};
  }

  size_t found_byte_start = offsets[found_char_idx];
  size_t found_char_end = found_char_idx + uni_search_lower.length();
  size_t found_byte_end;
  if (found_char_end >= offsets.size()) {
    found_byte_end = text.length();
  } else {
    found_byte_end = offsets[found_char_end];
  }

  return {found_byte_start, found_byte_end};
}

/**
@brief Находит ID токена в словаре с учётом регистра.
@param token Строковое представление токена.
@return int64_t ID токена или ID [UNK], если не найден.
*/
int64_t SimpleTokenizer::find_token_in_vocab(const string &token) {
  // Прямой поиск
  auto it = vocab_map_.find(token);
  if (it != vocab_map_.end())
    return it->second;

  // Поиск в нижнем регистре (с учётом кириллицы через Unistring)
  string lower = utf8::Unistring(token).to_lower().to_string();
  it = vocab_map_.find(lower);
  if (it != vocab_map_.end())
    return it->second;

  return get_unk_token_id();
}

// --- Основная логика кодирования ---

/**
@brief Выполняет основную фазу кодирования: токенизация, поиск смещений,
word_ids. Не включает паддинг и special-токены — это делает pad_encoding.
@param text Исходный текст.
@param raw_tokens Вектор токенов после предварительной разбивки.
@param max_len Максимальная длина последовательности.
@return CoreEncoding Промежуточный результат.
*/
SimpleTokenizer::CoreEncoding
SimpleTokenizer::build_encoding_core(const std::string &text,
                                     const std::vector<std::string> &raw_tokens,
                                     size_t max_len) {
  CoreEncoding core;

  // Добавляем токен [CLS] в начало
  core.input_ids.push_back(get_cls_token_id());
  core.tokens.push_back(get_cls_token());
  core.offsets.push_back({0, 0});
  core.word_ids.push_back(-1);

  size_t current_pos = 0; // Текущая позиция в исходном тексте (в байтах)
  int word_id = 0;        // Счётчик слов для word_ids

  for (const std::string &raw_token : raw_tokens) {
    // Проверка на переполнение (оставляем место для [SEP])
    if (core.input_ids.size() >= max_len - 1)
      break;

    // Находим ID токена в словаре
    int64_t token_id = find_token_in_vocab(raw_token);
    string actual_token =
        id_to_token_.count(token_id) ? id_to_token_[token_id] : get_unk_token();

    // Находим смещения токена в исходном тексте
    auto [start, end] = find_token_in_text(text, raw_token, current_pos);

    // Добавляем токен в результат
    core.input_ids.push_back(token_id);
    core.tokens.push_back(actual_token);
    core.offsets.push_back({start, end});

    // Логика word_ids: субтокены (##) получают тот же ID, что и первое слово
    if (raw_token.substr(0, 2) == "##") {
      // Субтокен — принадлежит предыдущему слову
      if (!core.word_ids.empty()) {
        core.word_ids.push_back(core.word_ids.back());
      } else {
        core.word_ids.push_back(-1);
      }
    } else {
      // Новое слово — увеличиваем счётчик
      core.word_ids.push_back(word_id++);
    }

    // Обновляем позицию для следующего поиска
    current_pos = end;
    // Пропускаем пробелы в исходном тексте
    while (current_pos < text.size() &&
           std::isspace(static_cast<unsigned char>(text[current_pos]))) {
      current_pos++;
    }
  }

  core.last_byte_pos = current_pos;
  return core;
}

/**
@brief Добавляет специальные токены и паддинг к промежуточному результату.
@param core Ссылка на CoreEncoding (изменяется на месте).
@param max_len Максимальная длина выходной последовательности.
*/
void SimpleTokenizer::pad_encoding(CoreEncoding &core, size_t max_len) {
  // Добавляем токен [SEP] в конец
  if (core.input_ids.size() < max_len) {
    core.input_ids.push_back(get_sep_token_id());
    core.tokens.push_back(get_sep_token());
    core.offsets.push_back({core.last_byte_pos, core.last_byte_pos});
    core.word_ids.push_back(-1);
  }

  // Добавляем паддинг до max_len
  while (core.input_ids.size() < max_len) {
    core.input_ids.push_back(get_pad_token_id());
    core.tokens.push_back(get_pad_token());
    core.offsets.push_back({0, 0});
    core.word_ids.push_back(-1);
  }
}

/**
@brief Публичный метод encode: объединяет все этапы кодирования.
@param text Исходный текст.
@param max_len Максимальная длина выходной последовательности.
@return EncodingResult Полный результат кодирования.
*/
SimpleTokenizer::EncodingResult SimpleTokenizer::encode(const string &text,
                                                        size_t max_len) {
  // 1. Токенизация
  std::vector<std::string> raw_tokens = split_text_into_tokens(text);

  // 2. Основная фаза: ID, токены, смещения, word_ids
  SimpleTokenizer::CoreEncoding core =
      build_encoding_core(text, raw_tokens, max_len);

  // 3. Паддинг и special-токены
  pad_encoding(core, max_len);

  // 4. Формируем финальный результат с маской внимания
  EncodingResult result;
  result.input_ids = std::move(core.input_ids);
  result.tokens = std::move(core.tokens);
  result.offsets = std::move(core.offsets);
  result.word_ids = std::move(core.word_ids);

  // Маска внимания: 1 для реальных токенов, 0 для [PAD]
  result.attention_mask.resize(max_len);
  for (size_t i = 0; i < max_len; ++i) {
    result.attention_mask[i] =
        (result.input_ids[i] != get_pad_token_id()) ? 1 : 0;
  }

  return result;
}

/**
@brief Декодирует последовательность ID токенов обратно в текст.
@param ids Вектор ID токенов.
@return string Восстановленный текст.
*/
string SimpleTokenizer::decode(const vector<int64_t> &ids) {
  string text;

  for (int64_t id : ids) {
    // Пропускаем специальные токены
    if (id == get_cls_token_id() || id == get_sep_token_id() ||
        id == get_pad_token_id())
      continue;

    string token = id_to_token_.count(id) ? id_to_token_[id] : "[UNK]";

    if (token == "[UNK]") {
      if (!text.empty() && text.back() != ' ')
        text += " ";
      text += "[UNK]";
    } else if (token.substr(0, 2) == "##") {
      // Субтокен — приклеиваем без пробела
      text += token.substr(2);
    } else {
      // Новое слово — добавляем пробел, если нужно
      if (!text.empty() && text.back() != ' ')
        text += " ";
      text += token;
    }
  }

  return text;
}

/**
@brief Токенизирует текст с возвратом смещений (для ONNX).
@param text Входной текст.
@param max_len Максимальная длина последовательности.
@return TokenizationResult Результат токенизации.
*/
SimpleTokenizer::TokenizationResult
SimpleTokenizer::tokenize_with_offsets(const string &text, size_t max_len) {
  SimpleTokenizer::EncodingResult enc = encode(text, max_len);
  return {enc.input_ids, enc.attention_mask, enc.tokens, enc.offsets};
}

/**
@brief Токенизирует текст без дополнительной информации.
@param text Входной текст.
@return vector<string> Вектор токенов.
*/
vector<string> SimpleTokenizer::tokenize_text(const string &text) {
  return encode(text, 512).tokens;
}
