#include "bert_onnx_inference.hpp"
#include "cJSON.h"
#include "simple_tokenizer.hpp"
#include "text_splitter.hpp"
#include "unistring.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

struct WordInfo {
  std::string text;
  size_t start;
  size_t end;
  std::vector<std::string> labels;
  std::string main_label;
  bool has_b_prefix;
};

/**
 * @brief Проверка на знак препинания
 * @param text Текст для проверки
 * @return true если текст является знаком препинания
 */
bool is_punctuation(const std::string &text) {
  utf8::Unistring uni(text);
  return utf8::TextSplitter::isPunctuation(uni);
}

/**
 * @brief Получение базовой метки без BIO-префикса
 * @param label Метка с BIO-префиксом
 * @return Базовая метка без префикса
 */
std::string get_base_label(const std::string &label) {
  if (label.size() >= 2 &&
      (label.substr(0, 2) == "B-" || label.substr(0, 2) == "I-")) {
    return label.substr(2);
  }
  return label;
}

/**
 * @brief Определение основной метки для слова (B- имеет приоритет над I-)
 * @param labels Вектор меток
 * @return Основная метка
 */
std::string determine_main_label(const std::vector<std::string> &labels) {
  if (labels.empty()) {
    return "O";
  }
  // Сначала ищем B-* метки
  for (const auto &lbl : labels) {
    if (lbl.substr(0, 2) == "B-") {
      return lbl;
    }
  }
  // Затем I-* метки
  for (const auto &lbl : labels) {
    if (lbl.substr(0, 2) == "I-") {
      return lbl;
    }
  }
  return labels[0];
}

/**
 * @brief Создание сущности из информации о слове
 * @param word Информация о слове
 * @param original_text Исходный текст предложения
 * @param labels_map Карта меток
 * @return Созданная сущность
 */
Entity createEntity(
    const WordInfo &word, const std::string &original_text,
    const std::map<int, std::pair<std::string, std::string>> &labels_map) {
  Entity entity;

  // Извлекаем оригинальный текст из исходного предложения по смещениям
  if (word.start < original_text.length() &&
      word.end <= original_text.length()) {
    entity.text = original_text.substr(word.start, word.end - word.start);
  } else {
    entity.text = word.text;
  }

  entity.start = word.start;
  entity.end = word.end;

  // Очищаем текст от пунктуации для entity.text
  utf8::Unistring uni_word(entity.text);
  utf8::Unistring cleaned_word = utf8::TextSplitter::cleanWord(uni_word);
  std::string clean_text = cleaned_word.to_string();

  // Если после очистки осталось пустым, используем word.text
  if (clean_text.empty()) {
    clean_text = word.text;
  }

  // Находим русское название метки
  for (const auto &lbl_pair : labels_map) {
    if (lbl_pair.second.first == word.main_label) {
      entity.type = lbl_pair.second.first;
      entity.type_ru = lbl_pair.second.second;
      break;
    }
  }

  return entity;
}

} // namespace

BertOnnxInference::BertOnnxInference(
    std::unique_ptr<onnx_infer::BertNerModel> model,
    std::shared_ptr<SimpleTokenizer> tokenizer,
    const std::map<int, std::pair<std::string, std::string>> &labels,
    size_t max_len)
    : model_(std::move(model)), tokenizer_(std::move(tokenizer)),
      labels_(labels), max_len_(max_len) {}

std::vector<std::string>
BertOnnxInference::split_into_sentences(const std::string &text) {
  std::vector<std::string> sentences;

  // Используем новый TextSplitter для корректного разбиения на предложения
  utf8::Unistring uni_text(text);
  std::vector<utf8::Unistring> uni_sentences =
      utf8::TextSplitter::splitIntoSentences(uni_text);

  for (const auto &uni_sent : uni_sentences) {
    sentences.push_back(uni_sent.to_string());
  }

  return sentences;
}

SentenceResult
BertOnnxInference::process_sentence(const std::string &sentence) {
  SentenceResult result;
  result.text = sentence;

  // Токенизация
  SimpleTokenizer::EncodingResult encoding =
      tokenizer_->encode(sentence, max_len_);

  if (encoding.input_ids.empty()) {
    std::cout << "input_ids пуст" << std::endl;
    return result;
  }

  // Получаем предсказания модели
  std::vector<int> predictions =
      model_->predict_labels(encoding.input_ids, encoding.attention_mask);

  // Сохраняем токены и метки
  result.tokens = encoding.tokens;
  result.token_labels = predictions;

  // Объединяем подслова в сущности
  result.entities =
      merge_subwords(encoding.tokens, predictions, encoding.offsets, sentence);

  return result;
}

/**
 * @brief Группировка токенов в слова
 * @param tokens Токены
 * @param token_labels Метки токенов
 * @param offsets Смещения токенов
 * @param labels_map Карта меток
 * @return Вектор информации о словах
 */
std::vector<WordInfo> group_tokens_into_words(
    const std::vector<std::string> &tokens,
    const std::vector<int> &token_labels,
    const std::vector<std::pair<size_t, size_t>> &offsets,
    const std::map<int, std::pair<std::string, std::string>> &labels_map) {

  std::vector<WordInfo> words;
  std::string current_word;
  size_t current_start = 0;
  size_t current_end = 0;
  std::vector<std::string> current_labels;
  bool in_word = false;

  for (size_t i = 0; i < tokens.size(); ++i) {
    const std::string &token = tokens[i];

    // Пропускаем специальные токены
    if (token == "[CLS]" || token == "[SEP]" || token == "[PAD]") {
      continue;
    }

    // Проверяем, существует ли метка
    auto label_it = labels_map.find(token_labels[i]);
    if (label_it == labels_map.end()) {
      continue;
    }

    std::string label = label_it->second.first;
    bool is_subword = (token.substr(0, 2) == "##");
    std::string clean_token = is_subword ? token.substr(2) : token;

    size_t start = offsets[i].first;
    size_t end = offsets[i].second;

    // Проверка на корректность смещений
    if (start > end) {
      std::cerr << "Warning: Invalid offsets for token '" << token
                << "': start=" << start << ", end=" << end << std::endl;
      std::swap(start, end);
    }

    // Пропускаем токены с нулевыми смещениями (только если это не начало
    // текста)
    if (start == 0 && end == 0) {
      // Разрешаем токены в начале строки (i == 0 или предыдущий токен тоже имел
      // 0,0)
      if (i > 0 && offsets[i - 1].second != 0) {
        continue;
      }
    }

    if (is_subword && in_word) {
      // Продолжаем текущее слово (субтокен присоединяется без пробела)
      current_word += clean_token;
      current_end = end;
      current_labels.push_back(label);
    } else {
      // Завершаем предыдущее слово
      if (in_word && !current_word.empty()) {
        std::string main_label = determine_main_label(current_labels);
        bool has_b = std::any_of(
            current_labels.begin(), current_labels.end(),
            [](const std::string &lbl) { return lbl.substr(0, 2) == "B-"; });

        words.push_back({current_word, current_start, current_end,
                         current_labels, main_label, has_b});
      }

      // Начинаем новое слово
      current_word = clean_token;
      current_start = start;
      current_end = end;
      current_labels = {label};
      in_word = true;
    }
  }

  // Добавляем последнее слово
  if (in_word && !current_word.empty()) {
    std::string main_label = determine_main_label(current_labels);
    bool has_b = std::any_of(
        current_labels.begin(), current_labels.end(),
        [](const std::string &lbl) { return lbl.substr(0, 2) == "B-"; });

    words.push_back({current_word, current_start, current_end, current_labels,
                     main_label, has_b});
  }

  return words;
}

/**
 * @brief Группировка слов в фразы (сущности) с дополнительными правилами
 * @param words Вектор информации о словах
 * @param labels_map Карта меток
 * @param original_text Исходный текст
 * @return Вектор сущностей
 */
std::vector<Entity> group_words_into_phrases(
    const std::vector<WordInfo> &words,
    const std::map<int, std::pair<std::string, std::string>> &labels_map,
    const std::string &original_text) {
  std::vector<Entity> entities;

  if (words.empty()) {
    return entities;
  }

  // Вспомогательная функция для проверки, является ли слово предлогом
  auto is_preposition = [](const std::string &word_text) -> bool {
    static const std::vector<std::string> prepositions = {
        "в",      "во",  "без",   "до",   "для",   "за",    "из",    "из-за",
        "из-под", "к",   "ко",    "на",   "над",   "о",     "об",    "обо",
        "от",     "ото", "по",    "под",  "подо",  "при",   "про",   "с",
        "со",     "у",   "через", "чрез", "между", "перед", "передо"};
    std::string lower_word = utf8::Unistring(word_text).to_lower().to_string();
    return std::find(prepositions.begin(), prepositions.end(), lower_word) !=
           prepositions.end();
  };

  // Вспомогательная функция для проверки, начинается ли слово с заглавной буквы
  auto is_capitalized = [](const std::string &word_text) -> bool {
    if (word_text.empty())
      return false;
    utf8::Unistring uni_word(word_text);
    if (uni_word.length() == 0)
      return false;
    utf8::Unistring first_char = uni_word[0];
    std::string first_char_str = first_char.to_string();

    // Проверка на заглавную кириллическую букву
    if (first_char_str.size() == 2) {
      unsigned char c1 = static_cast<unsigned char>(first_char_str[0]);
      unsigned char c2 = static_cast<unsigned char>(first_char_str[1]);
      // А-Я в UTF-8: D0 90-9F, Ё: D0 81
      if (c1 == 0xD0) {
        return (c2 >= 0x90 && c2 <= 0x9F) || c2 == 0x81;
      }
    }
    // ASCII заглавные
    return std::isupper(static_cast<unsigned char>(first_char_str[0]));
  };

  // Вспомогательная функция для получения базовой метки (без BIO-префикса)
  auto get_base = [](const std::string &label) -> std::string {
    if (label.size() >= 2 &&
        (label.substr(0, 2) == "B-" || label.substr(0, 2) == "I-")) {
      return label.substr(2);
    }
    return label;
  };

  // Вспомогательная функция для проверки знака препинания
  auto is_punct = [](const std::string &text) -> bool {
    utf8::Unistring uni(text);
    return utf8::TextSplitter::isPunctuation(uni);
  };

  // Первый проход: предварительное объединение в сущности (без правил)
  std::vector<Entity> raw_entities;
  Entity *current_entity = nullptr;

  for (const WordInfo &word : words) {
    // Пропускаем знаки препинания
    if (is_punct(word.text)) {
      continue;
    }

    std::string base_label = get_base(word.main_label);
    bool is_b_prefix =
        (word.main_label.size() >= 2 && word.main_label.substr(0, 2) == "B-");
    bool is_i_prefix =
        (word.main_label.size() >= 2 && word.main_label.substr(0, 2) == "I-");

    if (current_entity == nullptr) {
      raw_entities.push_back(createEntity(word, original_text, labels_map));
      current_entity = &raw_entities.back();
    } else {
      std::string current_base = get_base(current_entity->type);

      if (base_label == "O") {
        current_entity = nullptr;
        raw_entities.push_back(createEntity(word, original_text, labels_map));
        current_entity = &raw_entities.back();
      } else if (is_b_prefix) {
        current_entity = nullptr;
        raw_entities.push_back(createEntity(word, original_text, labels_map));
        current_entity = &raw_entities.back();
      } else if (is_i_prefix) {
        std::string word_base = get_base(word.main_label);
        size_t distance = word.start - current_entity->end;

        if (current_base == word_base && distance <= 2) {
          std::string original_word_text;
          if (word.start < original_text.length() &&
              word.end <= original_text.length()) {
            original_word_text =
                original_text.substr(word.start, word.end - word.start);
          } else {
            original_word_text = word.text;
          }
          current_entity->text += " " + original_word_text;
          current_entity->end = word.end;
        } else {
          current_entity = nullptr;
          raw_entities.push_back(createEntity(word, original_text, labels_map));
          current_entity = &raw_entities.back();
        }
      } else {
        current_entity = nullptr;
        raw_entities.push_back(createEntity(word, original_text, labels_map));
        current_entity = &raw_entities.back();
      }
    }
  }

  // Второй проход: применение дополнительных правил к raw_entities

  // Правило 1: если между предлогом и обстоятельством есть определение,
  // всех их нужно считать одним обстоятельством
  for (size_t i = 0; i + 2 < raw_entities.size(); ++i) {
    Entity &first = raw_entities[i];
    Entity &second = raw_entities[i + 1];
    Entity &third = raw_entities[i + 2];

    std::string first_base = get_base(first.type);
    std::string second_base = get_base(second.type);
    std::string third_base = get_base(third.type);

    bool first_is_prep = is_preposition(first.text);
    bool second_is_definition = (second_base == "DEFINITION");
    bool third_is_adverbial = (third_base == "ADVERBIAL");

    if (first_is_prep && second_is_definition && third_is_adverbial) {
      // Проверяем расстояние между сущностями
      size_t dist1 = second.start - first.end;
      size_t dist2 = third.start - second.end;

      if (dist1 <= 3 && dist2 <= 3) {
        // Объединяем предлог, определение и обстоятельство в одну сущность
        std::string merged_text =
            first.text + " " + second.text + " " + third.text;
        Entity merged;
        merged.text = merged_text;
        merged.type = "B-ADVERBIAL";
        merged.type_ru = "обстоятельство";
        merged.start = first.start;
        merged.end = third.end;

        // Заменяем три сущности одной
        raw_entities.erase(raw_entities.begin() + i,
                           raw_entities.begin() + i + 3);
        raw_entities.insert(raw_entities.begin() + i, merged);
        i--; // Корректируем индекс после удаления
      }
    }
  }

  // Правило 2: объединение слов с заглавной буквы
  for (size_t i = 0; i < raw_entities.size(); ++i) {
    // Ищем последовательность сущностей, начинающихся с заглавной буквы
    std::vector<size_t> capital_sequence = {i};
    size_t j = i + 1;

    while (j < raw_entities.size()) {
      // Получаем первое слово текущей сущности
      std::string first_word_of_current = raw_entities[j - 1].text;
      size_t space_pos = first_word_of_current.find(' ');
      if (space_pos != std::string::npos) {
        first_word_of_current = first_word_of_current.substr(0, space_pos);
      }

      // Получаем первое слово следующей сущности
      std::string first_word_of_next = raw_entities[j].text;
      space_pos = first_word_of_next.find(' ');
      if (space_pos != std::string::npos) {
        first_word_of_next = first_word_of_next.substr(0, space_pos);
      }

      // Проверяем расстояние
      size_t distance = raw_entities[j].start - raw_entities[j - 1].end;

      if (is_capitalized(first_word_of_next) && distance <= 3) {
        capital_sequence.push_back(j);
        j++;
      } else {
        break;
      }
    }

    // Если нашли последовательность из 2+ слов с заглавной
    if (capital_sequence.size() >= 2) {
      // Проверяем, есть ли среди них подлежащее или дополнение
      bool has_subject = false;
      bool has_addition = false;

      for (size_t idx : capital_sequence) {
        std::string type_base = get_base(raw_entities[idx].type);
        if (type_base == "SUBJECT")
          has_subject = true;
        if (type_base == "ADDITION")
          has_addition = true;
      }

      std::string target_type;
      std::string target_type_ru;
      if (has_subject) {
        target_type = "B-SUBJECT";
        target_type_ru = "подлежащее";
      } else if (has_addition) {
        target_type = "B-ADDITION";
        target_type_ru = "дополнение";
      } else {
        // Если нет ни подлежащего, ни дополнения, пропускаем объединение
        i = j;
        continue;
      }

      // Объединяем все сущности в последовательности
      std::string merged_text;
      size_t merged_start = raw_entities[capital_sequence[0]].start;
      size_t merged_end = raw_entities[capital_sequence.back()].end;

      for (size_t idx : capital_sequence) {
        if (!merged_text.empty())
          merged_text += " ";
        merged_text += raw_entities[idx].text;
      }

      Entity merged;
      merged.text = merged_text;
      merged.type = target_type;
      merged.type_ru = target_type_ru;
      merged.start = merged_start;
      merged.end = merged_end;

      // Заменяем последовательность одной сущностью
      raw_entities.erase(raw_entities.begin() + i, raw_entities.begin() + j);
      raw_entities.insert(raw_entities.begin() + i, merged);

      // Переходим к следующей после объединенной сущности
      i = i; // Остаемся на той же позиции (новая объединенная сущность)
    } else {
      i = j;
    }
  }

  // Правило 3: предлог перед обстоятельством или дополнением -> всё
  // обстоятельство
  for (size_t i = 0; i + 1 < raw_entities.size(); ++i) {
    Entity &first = raw_entities[i];
    Entity &second = raw_entities[i + 1];

    std::string second_base = get_base(second.type);

    bool first_is_prep = is_preposition(first.text);
    bool second_is_adverbial = (second_base == "ADVERBIAL");
    bool second_is_addition = (second_base == "ADDITION");
    size_t distance = second.start - first.end;

    if (first_is_prep && (second_is_adverbial || second_is_addition) &&
        distance <= 3) {
      // Объединяем предлог и обстоятельство/дополнение в обстоятельство
      std::string merged_text = first.text + " " + second.text;
      Entity merged;
      merged.text = merged_text;
      merged.type = "B-ADVERBIAL";
      merged.type_ru = "обстоятельство";
      merged.start = first.start;
      merged.end = second.end;

      raw_entities.erase(raw_entities.begin() + i,
                         raw_entities.begin() + i + 2);
      raw_entities.insert(raw_entities.begin() + i, merged);
      i--; // Корректируем индекс
    }
  }

  // Сохраняем все сущности, включая "O" (другое)
  // НО: для "O" нужно правильно установить русское название
  for (const Entity &entity : raw_entities) {
    Entity final_entity = entity;

    // Если тип сущности "O" или её базовая метка "O", устанавливаем правильное
    // русское название
    std::string base_type = get_base(entity.type);
    if (base_type == "O") {
      final_entity.type_ru = "другое";
      // Убеждаемся, что type содержит правильный BIO-формат
      if (final_entity.type == "O" || final_entity.type.empty()) {
        final_entity.type = "O";
      }
    }

    entities.push_back(final_entity);
  }

  return entities;
}

std::vector<Entity> BertOnnxInference::merge_subwords(
    const std::vector<std::string> &tokens,
    const std::vector<int> &token_labels,
    const std::vector<std::pair<size_t, size_t>> &offsets,
    const std::string &original_text) {
  // Группировка токенов в слова
  std::vector<WordInfo> words =
      group_tokens_into_words(tokens, token_labels, offsets, labels_);

  // Группировка слов в фразы
  return group_words_into_phrases(words, labels_, original_text);
}

/**
 * @brief Загрузка меток из config.json
 * @param path Путь к файлу config.json
 * @return Карта меток (ID -> {английское_название, русское_название})
 */
std::map<int, std::pair<std::string, std::string>>
load_labels(const std::string &path) {
  std::map<int, std::pair<std::string, std::string>> labels;

#ifdef _MSC_VER
  FILE *file = nullptr;
  fopen_s(&file, path.c_str(), "rb");
#else
  FILE *file = fopen(path.c_str(), "rb");
#endif
  if (!file)
    return labels;

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  std::vector<char> buffer(size + 1);
  fread(buffer.data(), 1, size, file);
  fclose(file);

  cJSON *config = cJSON_Parse(buffer.data());
  if (!config)
    return labels;

  cJSON *id2label = cJSON_GetObjectItem(config, "id2label");
  if (id2label && id2label->type == cJSON_Object) {
    for (cJSON *child = id2label->child; child; child = child->next) {
      int id = std::stoi(child->string);
      std::string eng = child->valuestring;
      std::string rus;

      if (eng == "O")
        rus = "другое";
      else if (eng.find("SUBJECT") != std::string::npos)
        rus = "подлежащее";
      else if (eng.find("PREDICATE") != std::string::npos)
        rus = "сказуемое";
      else if (eng.find("DEFINITION") != std::string::npos)
        rus = "определение";
      else if (eng.find("ADDITION") != std::string::npos)
        rus = "дополнение";
      else if (eng.find("ADVERBIAL") != std::string::npos)
        rus = "обстоятельство";
      else
        rus = eng;

      labels[id] = {eng, rus};
    }
  }

  cJSON_Delete(config);
  return labels;
}

/**
 * @brief Извлечение членов предложения из текста
 * @param text Входной текст
 * @return std::vector<SentenceResult> Результаты по предложениям
 */
std::vector<SentenceResult>
BertOnnxInference::extract_sentence_parts(const std::string &text) {
  std::vector<SentenceResult> results;

  // Разбиваем на предложения
  std::vector<std::string> sentences = split_into_sentences(text);

  for (const std::string &sentence : sentences) {
    // if (sentence.length() < 3)
    // continue; // Пропускаем слишком короткие

    SentenceResult result = process_sentence(sentence);
    results.push_back(result);
  }

  return results;
}
