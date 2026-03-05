#include "bert_onnx_inference.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <regex>

using namespace std;
using namespace chrono;

/**
 * @brief Статический список меток для NER (Named Entity Recognition)
 *
 * Содержит все возможные метки сущностей в формате BIO:
 * - O: Outside (не сущность)
 * - B-...: Beginning (начало сущности типа XXX)
 * - I-...: Inside (внутри сущности типа XXX)
 */
const vector<string> BertOnnxInference::LABELS = {
    "O",         "B-MANNER",     "I-MANNER",     "B-TIME",      "I-TIME",
    "B-DEGREE",  "I-DEGREE",     "B-CONDITION",  "I-CONDITION", "B-CAUSE",
    "I-CAUSE",   "B-CONCESSION", "I-CONCESSION", "B-LOCATION",  "I-LOCATION",
    "B-PURPOSE", "I-PURPOSE"};

/**
 * @brief Конструктор класса BertOnnxInference
 *
 * Инициализирует ONNX Runtime среду, загружает токенизатор и модель BERT для
 * NER. Выполняет валидацию загруженной модели.
 *
 * @param model_path Путь к файлу ONNX модели
 * @param vocab_path Путь к файлу словаря токенизатора
 * @throws runtime_error Если не удается загрузить токенизатор или модель
 */
BertOnnxInference::BertOnnxInference(const string &model_path,
                                     const string &vocab_path)
    : env(ORT_LOGGING_LEVEL_WARNING, "BERT_NER") {

  cout << "Initializing BERT NER model..." << endl;

  // Загрузка токенизатора
  try {
    tokenizer = make_unique<SimpleTokenizer>(vocab_path);
    cout << "✓ Tokenizer loaded" << endl;
  } catch (const exception &e) {
    throw runtime_error("Failed to load tokenizer: " + string(e.what()));
  }

  // Загрузка модели ONNX
  Ort::SessionOptions session_options;
  session_options.SetIntraOpNumThreads(1);
  session_options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_ALL);

  try {
    session = Ort::Session(env, model_path.c_str(), session_options);
    cout << "✓ ONNX model loaded: " << model_path << endl;

    // Валидация модели
    validate_model();

  } catch (const exception &e) {
    throw runtime_error("Failed to load ONNX model: " + string(e.what()));
  }

  // Инициализация статистики
  stats_ = Stats();
}

/**
 * @brief Деструктор класса BertOnnxInference
 *
 * Автоматически освобождает ресурсы через умные указатели
 */
BertOnnxInference::~BertOnnxInference() {
  // Ресурсы очищаются автоматически
}

/**
 * @brief Валидация загруженной ONNX модели
 *
 * Проверяет соответствие модели ожидаемым параметрам.
 * Выводит предупреждение, если количество входов отличается от ожидаемого.
 */
void BertOnnxInference::validate_model() {
  Ort::AllocatorWithDefaultOptions allocator;

  // Проверяем, что модель соответствует ожиданиям
  if (session.GetInputCount() != 2) {
    cerr << "  Warning: Model expects " << session.GetInputCount()
         << " inputs, expected 2" << endl;
  }
}

/**
 * @brief Разделяет текст на отдельные предложения
 *
 * Использует регулярные выражения для разделения по знакам препинания (.!?)
 *
 * @param text Входной текст для разделения
 * @return vector<string> Вектор предложений
 */
vector<string> BertOnnxInference::split_into_sentences(const string &text) {
  vector<string> sentences;

  if (text.empty()) {
    return sentences;
  }

  // Улучшенное разделение по знакам препинания
  regex sentence_regex(R"(([^.!?]+[.!?]+)\s*)");

  sregex_iterator it(text.begin(), text.end(), sentence_regex);
  sregex_iterator end;

  for (; it != end; ++it) {
    string sentence = it->str(1);

    // Убираем пробелы в начале и конце
    sentence.erase(0, sentence.find_first_not_of(" \t\n\r\f\v"));
    sentence.erase(sentence.find_last_not_of(" \t\n\r\f\v") + 1);

    if (!sentence.empty()) {
      sentences.push_back(sentence);
    }
  }

  // Если не нашли предложений (нет знаков препинания),
  // возвращаем весь текст как одно предложение
  if (sentences.empty() && !text.empty()) {
    sentences.push_back(text);
  }

  return sentences;
}

/**
 * @brief Группирует токены в сущности на основе BIO меток
 *
 * Объединяет последовательности токенов с метками B-* и I-* в цельные сущности,
 * восстанавливая их текстовое представление из исходного текста.
 *
 * @param tokenization Результат токенизации с информацией о смещениях
 * @param labels Вектор предсказанных BIO меток для каждого токена
 * @param original_text Исходный текст для извлечения текста сущностей
 * @return vector<Entity> Вектор выделенных сущностей
 */
vector<BertOnnxInference::Entity> BertOnnxInference::group_entities(
    const SimpleTokenizer::EncodingResult &tokenization,
    const vector<string> &labels, const string &original_text) {

  vector<Entity> entities;
  Entity current_entity;
  bool in_entity = false;
  int last_word_id = -1;

  for (size_t i = 0; i < tokenization.tokens.size(); i++) {
    string token = tokenization.tokens[i];
    string label = labels[i];
    int word_id = tokenization.word_ids[i];

    // Пропускаем специальные токены
    if (token == tokenizer->get_cls_token() ||
        token == tokenizer->get_sep_token() ||
        token == tokenizer->get_pad_token()) {
      if (in_entity) {
        entities.push_back(current_entity);
        in_entity = false;
      }
      continue;
    }

    // Проверяем, начало ли это нового слова
    bool new_word = (word_id != last_word_id);
    last_word_id = word_id;

    if (label.substr(0, 2) == "B-") {
      // Начало новой сущности
      if (in_entity) {
        entities.push_back(current_entity);
      }

      current_entity.type = label.substr(2);
      current_entity.start = tokenization.offsets[i].first;
      current_entity.end = tokenization.offsets[i].second;
      current_entity.tokens = {token};

      // Извлекаем текст сущности из оригинала
      if (current_entity.start < original_text.size() &&
          current_entity.end <= original_text.size()) {
        current_entity.text = original_text.substr(
            current_entity.start, current_entity.end - current_entity.start);
      } else {
        current_entity.text = token;
      }

      in_entity = true;

    } else if (label.substr(0, 2) == "I-") {
      // Продолжение сущности
      if (in_entity && label.substr(2) == current_entity.type) {
        // Добавляем токен
        current_entity.tokens.push_back(token);

        // Обновляем позицию конца
        current_entity.end = tokenization.offsets[i].second;

        // Обновляем текст сущности
        if (current_entity.start < original_text.size() &&
            current_entity.end <= original_text.size()) {
          current_entity.text = original_text.substr(
              current_entity.start, current_entity.end - current_entity.start);
        }
      } else {
        // Несоответствие меток
        if (in_entity) {
          entities.push_back(current_entity);
        }
        in_entity = false;
      }
    } else {
      // Метка "O" - конец сущности
      if (in_entity) {
        entities.push_back(current_entity);
        in_entity = false;
      }
    }
  }

  // Добавляем последнюю сущность если она есть
  if (in_entity) {
    entities.push_back(current_entity);
  }

  return entities;
}

/**
 * @brief Выполняет инференс ONNX модели
 *
 * Подготавливает входные тензоры, запускает модель и возвращает логиты.
 *
 * @param input_ids Вектор ID токенов
 * @param attention_mask Вектор маски внимания
 * @return vector<float> Вектор логитов для всех токенов и классов
 * @throws runtime_error Если инференс завершился ошибкой
 */
vector<float>
BertOnnxInference::run_inference(const vector<int64_t> &input_ids,
                                 const vector<int64_t> &attention_mask) {

  // Подготовка входных тензоров
  Ort::MemoryInfo memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  // Размеры входных данных
  vector<int64_t> input_ids_shape = {1, static_cast<int64_t>(input_ids.size())};
  vector<int64_t> attention_mask_shape = {
      1, static_cast<int64_t>(attention_mask.size())};

  // Создаем тензор input_ids как INT64
  Ort::Value input_ids_tensor = Ort::Value::CreateTensor<int64_t>(
      memory_info, const_cast<int64_t *>(input_ids.data()), input_ids.size(),
      input_ids_shape.data(), input_ids_shape.size());

  // Конвертируем attention_mask из int64 в float
  vector<float> attention_mask_float(attention_mask.begin(),
                                     attention_mask.end());

  // Создаем тензор attention_mask как FLOAT
  Ort::Value attention_mask_tensor = Ort::Value::CreateTensor<float>(
      memory_info, attention_mask_float.data(), attention_mask_float.size(),
      attention_mask_shape.data(), attention_mask_shape.size());

  // Имена входов и выходов
  vector<const char *> input_names = {"input_ids", "attention_mask"};
  vector<const char *> output_names = {"logits"};

  vector<Ort::Value> inputs;
  inputs.push_back(move(input_ids_tensor));
  inputs.push_back(move(attention_mask_tensor));

  // Выполнение модели
  try {
    vector<Ort::Value> outputs =
        session.Run(Ort::RunOptions{nullptr}, input_names.data(), inputs.data(),
                    inputs.size(), output_names.data(), output_names.size());

    // Получение выходных данных
    Ort::Value &logits_tensor = outputs[0];
    std::vector<int64_t> logits_shape =
        logits_tensor.GetTensorTypeAndShapeInfo().GetShape();

    // Получаем указатель на данные
    float *logits_data = logits_tensor.GetTensorMutableData<float>();
    size_t total_elements = 1;
    for (long dim : logits_shape) {
      if (dim > 0) {
        total_elements *= dim;
      }
    }

    // Копируем данные в вектор
    vector<float> logits(logits_data, logits_data + total_elements);

    return logits;

  } catch (const Ort::Exception &e) {
    cerr << "  ✗ ONNX Runtime error: " << e.what() << endl;
    cerr << "  Error code: " << e.GetOrtErrorCode() << endl;
    throw runtime_error("Inference failed: " + string(e.what()));
  }
}

/**
 * @brief Обновляет статистику обработки
 *
 * Увеличивает счетчики общего количества сущностей и
 * счетчики для каждого типа сущностей.
 *
 * @param entities Вектор найденных сущностей
 */
void BertOnnxInference::update_stats(const vector<Entity> &entities) {
  stats_.total_entities += entities.size();

  for (const auto &entity : entities) {
    stats_.entity_counts[entity.type]++;
  }
}

/**
 * @brief Основной метод извлечения обстоятельств из текста
 *
 * Разбивает текст на предложения, для каждого предложения выполняет
 * токенизацию, инференс модели и группировку сущностей.
 * Собирает статистику по времени выполнения.
 *
 * @param text Входной текст для анализа
 * @return vector<SentenceResult> Результаты для каждого предложения
 */
vector<BertOnnxInference::SentenceResult>
BertOnnxInference::extract_circumstances(const string &text) {

  vector<SentenceResult> results;

  if (text.empty()) {
    return results;
  }

  time_point start_time = high_resolution_clock::now();

  // Разделяем текст на предложения
  auto sentences = split_into_sentences(text);
  stats_.total_sentences += sentences.size();

  for (const auto &sentence : sentences) {
    SentenceResult result;
    result.text = sentence;

    // Токенизация
    auto tokenization = tokenizer->encode(sentence, MAX_LEN);

    // Запуск модели
    vector<float> logits =
        run_inference(tokenization.input_ids, tokenization.attention_mask);

    // Преобразуем логиты в метки
    vector<string> predicted_labels;

    // logits: [batch_size=1, seq_len, num_labels=17]
    size_t seq_len = min(MAX_LEN, tokenization.tokens.size());
    size_t num_labels = LABELS.size();

    for (size_t i = 0; i < seq_len; i++) {
      float max_logit = -numeric_limits<float>::max();
      size_t max_index = 0;

      for (size_t j = 0; j < num_labels; j++) {
        size_t idx = i * num_labels + j;
        if (idx < logits.size() && logits[idx] > max_logit) {
          max_logit = logits[idx];
          max_index = j;
        }
      }

      if (max_index < LABELS.size()) {
        predicted_labels.push_back(LABELS[max_index]);
      } else {
        predicted_labels.push_back("O");
      }
    }

    // Группируем сущности
    result.entities = group_entities(tokenization, predicted_labels, sentence);

    // Обновляем статистику
    update_stats(result.entities);

    results.push_back(result);
  }

  auto end_time = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(end_time - start_time);

  return results;
}
