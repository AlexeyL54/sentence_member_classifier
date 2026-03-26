// src/back/bert_onnx_inference.cpp
#include "bert_onnx_inference.hpp"

BertOnnxInference::BertOnnxInference(
    std::unique_ptr<onnx_infer::BertNerModel> model, SimpleTokenizer &tokenizer,
    const std::map<int, std::pair<std::string, std::string>> &labels,
    size_t max_len)
    : model_(std::move(model)), tokenizer_(tokenizer), labels_(labels),
      max_len_(max_len) {}

std::vector<std::string>
BertOnnxInference::split_into_sentences(const std::string &text) {
  std::vector<std::string> sentences;
  std::string current_sentence;

  for (size_t i = 0; i < text.length(); ++i) {
    char c = text[i];
    current_sentence += c;

    // Простое разбиение по знакам препинания
    if (c == '.' || c == '!' || c == '?' || c == ';' || c == '\n') {
      // Убираем лишние пробелы
      while (!current_sentence.empty() && (current_sentence.back() == ' ' ||
                                           current_sentence.back() == '\n' ||
                                           current_sentence.back() == '\r' ||
                                           current_sentence.back() == '\t')) {
        current_sentence.pop_back();
      }

      if (!current_sentence.empty()) {
        sentences.push_back(current_sentence);
        current_sentence.clear();
      }

      // Пропускаем пробелы после знака препинания
      while (i + 1 < text.length() &&
             (text[i + 1] == ' ' || text[i + 1] == '\n' ||
              text[i + 1] == '\r' || text[i + 1] == '\t')) {
        i++;
      }
    }
  }

  // Добавляем последнее предложение, если оно не пустое
  if (!current_sentence.empty()) {
    while (!current_sentence.empty() &&
           (current_sentence.back() == ' ' || current_sentence.back() == '\n' ||
            current_sentence.back() == '\r' ||
            current_sentence.back() == '\t')) {
      current_sentence.pop_back();
    }
    if (!current_sentence.empty()) {
      sentences.push_back(current_sentence);
    }
  }

  return sentences;
}

BertOnnxInference::SentenceResult
BertOnnxInference::process_sentence(const std::string &sentence) {
  SentenceResult result;
  result.text = sentence;

  // Токенизация
  auto encoding = tokenizer_.encode(sentence, max_len_);

  if (encoding.input_ids.empty()) {
    return result;
  }

  // Получаем предсказания модели
  auto predictions =
      model_->predict_labels(encoding.input_ids, encoding.attention_mask);

  // Сохраняем токены и метки
  result.tokens = encoding.tokens;
  result.token_labels = predictions;

  // Объединяем подслова в сущности
  result.entities =
      merge_subwords(encoding.tokens, predictions, encoding.offsets, sentence);

  return result;
}

std::vector<BertOnnxInference::Entity> BertOnnxInference::merge_subwords(
    const std::vector<std::string> &tokens,
    const std::vector<int> &token_labels,
    const std::vector<std::pair<size_t, size_t>> &offsets,
    const std::string &original_text) {

  std::vector<Entity> entities;

  size_t i = 0;
  while (i < tokens.size()) {
    // Пропускаем специальные токены
    if (tokens[i] == "[CLS]" || tokens[i] == "[SEP]" || tokens[i] == "[PAD]") {
      i++;
      continue;
    }

    int current_label = token_labels[i];

    // Пропускаем O (не сущность)
    if (current_label == 0) { // O
      i++;
      continue;
    }

    // Проверяем, что метка существует в словаре
    auto label_it = labels_.find(current_label);
    if (label_it == labels_.end()) {
      i++;
      continue;
    }

    // Начинаем новую сущность
    Entity entity;
    entity.type = label_it->second.first;
    entity.type_ru = label_it->second.second;
    entity.start = offsets[i].first;

    // Собираем все части сущности
    std::string entity_text;
    size_t j = i;

    while (j < tokens.size()) {
      // Проверяем, является ли токен частью той же сущности
      int label = token_labels[j];

      // Для специальных токенов прерываем
      if (tokens[j] == "[CLS]" || tokens[j] == "[SEP]" ||
          tokens[j] == "[PAD]") {
        break;
      }

      // Проверяем метку
      auto current_label_it = labels_.find(label);
      if (current_label_it == labels_.end()) {
        j++;
        continue;
      }

      std::string token_type = current_label_it->second.first;

      // Для B-* начинаем новую сущность, для I-* продолжаем текущую
      if (j > i && (token_type.rfind("B-", 0) == 0)) {
        break;
      }

      // Добавляем текст
      std::string token = tokens[j];
      if (token.substr(0, 2) == "##") {
        entity_text += token.substr(2);
      } else {
        if (!entity_text.empty() && entity_text.back() != ' ') {
          entity_text += " ";
        }
        entity_text += token;
      }

      j++;
    }

    if (j > i) {
      entity.end = offsets[j - 1].second;
      entity.text = entity_text;

      // Упрощенная уверенность (можно улучшить)
      entity.confidence = 0.95f;

      entities.push_back(entity);
    }
    i = j;
  }

  return entities;
}

std::vector<BertOnnxInference::SentenceResult>
BertOnnxInference::extract_sentence_parts(const std::string &text) {
  std::vector<SentenceResult> results;

  // Разбиваем на предложения
  auto sentences = split_into_sentences(text);

  for (const auto &sentence : sentences) {
    if (sentence.length() < 3)
      continue; // Пропускаем слишком короткие

    auto result = process_sentence(sentence);
    results.push_back(result);
  }

  return results;
}
