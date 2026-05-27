// src/back/onnx_model.cpp
#include "onnx_model.hpp"
#include <cstdint>
#include <iostream>
#include <vector>

#ifdef _WIN32
#include <filesystem>
#endif

namespace onnx_infer {

/**
 * @brief Конструктор класса BertNerModel
 * @param model_path Путь к ONNX модели
 *
 * Загружает ONNX модель по указанному пути и инициализирует сессию ONNX
 * Runtime. При ошибке загрузки выбрасывает исключение.
 */
BertNerModel::BertNerModel(const std::string &model_path) {
  session_options_.SetIntraOpNumThreads(1);
  session_options_.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_ALL);

  try {
#ifdef _WIN32
    std::filesystem::path fs_path(model_path);
    std::wstring wide_model_path = fs_path.wstring();
    session_ = std::make_unique<Ort::Session>(env_, wide_model_path.c_str(),
                                              session_options_);
#else
    session_ = std::make_unique<Ort::Session>(env_, model_path.c_str(),
                                              session_options_);
#endif
    // Проверка входов модели
    size_t input_count = session_->GetInputCount();
    if (input_count != 2) {
      std::cerr << "[WARN] Model expects " << input_count
                << " inputs, expected 2\n";
    }

    // Проверка выходов модели
    size_t output_count = session_->GetOutputCount();
    if (output_count != 1) {
      std::cerr << "[WARN] Model has " << output_count
                << " outputs, expected 1\n";
    }

  } catch (const Ort::Exception &e) {
    std::cerr << "Failed to load ONNX model: " << e.what();
  }
}

/**
 * @brief Получить количество меток
 * @return Количество классов для NER (типы сущностей)
 */
size_t BertNerModel::num_labels() const { return num_labels_; }

/**
 * @brief Получить максимальную длину последовательности
 * @return Максимальная длина последовательности, поддерживаемая моделью
 */
size_t BertNerModel::max_seq_length() const { return max_seq_length_; }

/**
 * @brief Инференс модели
 * @param input_ids ID токенов (последовательность целых чисел)
 * @param attention_mask Маска внимания (1 для реальных токенов, 0 для padding)
 * @return Логиты для каждого токена [seq_len, num_labels]
 *
 * Выполняет прямой проход по модели и возвращает сырые логиты для каждого
 * токена.
 */
std::vector<std::vector<float>>
BertNerModel::predict(const std::vector<int64_t> &input_ids,
                      const std::vector<int64_t> &attention_mask) const {

  // Подготовка входных тензоров
  std::vector<Ort::Value> input_tensors =
      create_input_tensors(input_ids, attention_mask);

  // Запуск инференса
  std::vector<Ort::Value> output_tensors =
      session_->Run(Ort::RunOptions{nullptr}, input_names_,
                    input_tensors.data(), 2, output_names_, 1);

  return extract_logits_from_output(output_tensors);
}

/**
 * @brief Получение предсказанных меток
 * @param input_ids ID токенов (последовательность целых чисел)
 * @param attention_mask Маска внимания (1 для реальных токенов, 0 для padding)
 * @return Метки для каждого токена (индексы классов с максимальной
 * вероятностью)
 *
 * Выполняет инференс и возвращает предсказанные классы для каждого токена,
 * выбирая класс с максимальным значением логита.
 */
std::vector<int>
BertNerModel::predict_labels(const std::vector<int64_t> &input_ids,
                             const std::vector<int64_t> &attention_mask) const {

  std::vector<std::vector<float>> logits = predict(input_ids, attention_mask);

  std::vector<int> predictions;
  predictions.reserve(logits.size());

  for (const std::vector<float> &token_logits : logits) {
    int best_label = 0;
    float best_score = token_logits[0];
    for (size_t i = 1; i < token_logits.size(); ++i) {
      if (token_logits[i] > best_score) {
        best_score = token_logits[i];
        best_label = static_cast<int>(i);
      }
    }
    predictions.push_back(best_label);
  }

  return predictions;
}

/**
 * @brief Создать ONNX тензор для input_ids
 * @param input_ids Вектор ID токенов
 * @return ONNX тензор для входных ID токенов
 *
 * Преобразует вектор целых чисел в формат ONNX тензора с размерностью [1,
 * seq_len].
 */
Ort::Value BertNerModel::create_input_ids_tensor(
    const std::vector<int64_t> &input_ids) const {

  std::vector<int64_t> input_shape = {1,
                                      static_cast<int64_t>(input_ids.size())};

  Ort::MemoryInfo memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  return Ort::Value::CreateTensor<int64_t>(
      memory_info, const_cast<int64_t *>(input_ids.data()), input_ids.size(),
      input_shape.data(), input_shape.size());
}

/**
 * @brief Создать ONNX тензор для attention_mask
 * @param attention_mask Вектор маски внимания (int64)
 * @return ONNX тензор для маски внимания
 *
 * Преобразует маску внимания из int64 в float и создает ONNX тензор.
 */
Ort::Value BertNerModel::create_attention_mask_tensor(
    const std::vector<int64_t> &attention_mask) const {

  std::vector<int64_t> input_shape = {
      1, static_cast<int64_t>(attention_mask.size())};
  std::vector<float> attention_mask_float(attention_mask.begin(),
                                          attention_mask.end());

  Ort::MemoryInfo memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  return Ort::Value::CreateTensor<float>(
      memory_info, attention_mask_float.data(), attention_mask_float.size(),
      input_shape.data(), input_shape.size());
}

/**
 * @brief Создать все входные тензоры для модели
 * @param input_ids Вектор ID токенов
 * @param attention_mask Вектор маски внимания
 * @return Вектор из двух ONNX тензоров (input_ids, attention_mask)
 *
 * Утилитарный метод для создания полного набора входных тензоров.
 */
std::vector<Ort::Value> BertNerModel::create_input_tensors(
    const std::vector<int64_t> &input_ids,
    const std::vector<int64_t> &attention_mask) const {

  std::vector<Ort::Value> input_tensors;
  input_tensors.push_back(create_input_ids_tensor(input_ids));
  input_tensors.push_back(create_attention_mask_tensor(attention_mask));

  return input_tensors;
}

/**
 * @brief Извлечь логиты из выходных тензоров
 * @param output_tensors Вектор выходных тензоров от ONNX Runtime
 * @return Двумерный вектор логитов [seq_len, num_labels]
 *
 * Преобразует сырые данные из ONNX тензора в удобный для использования формат.
 */
std::vector<std::vector<float>> BertNerModel::extract_logits_from_output(
    std::vector<Ort::Value> &output_tensors) const {

  float *logits_ptr = output_tensors[0].GetTensorMutableData<float>();
  std::vector<int64_t> output_shape =
      output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

  // output_shape: [1, seq_len, num_labels]
  size_t seq_len = static_cast<size_t>(output_shape[1]);
  size_t num_labels = static_cast<size_t>(output_shape[2]);

  std::vector<std::vector<float>> result(seq_len,
                                         std::vector<float>(num_labels));

  for (size_t i = 0; i < seq_len; ++i) {
    for (size_t j = 0; j < num_labels; ++j) {
      result[i][j] = logits_ptr[i * num_labels + j];
    }
  }

  return result;
}

} // namespace onnx_infer
