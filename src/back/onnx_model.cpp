// src/back/onnx_model.cpp
#include "onnx_model.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace onnx_infer {

BertNerModel::BertNerModel(const std::string &model_path) {
  session_options_.SetIntraOpNumThreads(1);
  session_options_.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_ENABLE_ALL);

  try {
#ifdef _WIN32
    std::wstring wide_model_path(model_path.begin(), model_path.end());
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
    throw std::runtime_error(std::string("Failed to load ONNX model: ") +
                             e.what());
  }
}

std::vector<std::vector<float>>
BertNerModel::predict(const std::vector<int64_t> &input_ids,
                      const std::vector<int64_t> &attention_mask) const {

  // Создаем тензоры ONNX
  std::vector<int64_t> input_shape = {1,
                                      static_cast<int64_t>(input_ids.size())};

  Ort::MemoryInfo memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  Ort::Value input_ids_tensor = Ort::Value::CreateTensor<int64_t>(
      memory_info, const_cast<int64_t *>(input_ids.data()), input_ids.size(),
      input_shape.data(), input_shape.size());

  std::vector<float> attention_mask_float(attention_mask.begin(),
                                          attention_mask.end());
  Ort::Value attention_mask_tensor = Ort::Value::CreateTensor<float>(
      memory_info, attention_mask_float.data(), attention_mask_float.size(),
      input_shape.data(), input_shape.size());

  std::vector<Ort::Value> input_tensors;
  input_tensors.push_back(std::move(input_ids_tensor));
  input_tensors.push_back(std::move(attention_mask_tensor));

  const char *input_names[] = {"input_ids", "attention_mask"};
  const char *output_names[] = {"logits"};

  // Запуск инференса
  auto output_tensors = session_->Run(Ort::RunOptions{nullptr}, input_names,
                                      input_tensors.data(), 2, output_names, 1);

  // Извлечение logits
  float *logits_ptr = output_tensors[0].GetTensorMutableData<float>();
  auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();

  // output_shape: [1, seq_len, num_labels]
  size_t seq_len = output_shape[1];
  size_t num_labels = output_shape[2];

  std::vector<std::vector<float>> result(seq_len,
                                         std::vector<float>(num_labels));

  for (size_t i = 0; i < seq_len; ++i) {
    for (size_t j = 0; j < num_labels; ++j) {
      result[i][j] = logits_ptr[i * num_labels + j];
    }
  }

  return result;
}

std::vector<int>
BertNerModel::predict_labels(const std::vector<int64_t> &input_ids,
                             const std::vector<int64_t> &attention_mask) const {

  std::vector<std::vector<float>> logits = predict(input_ids, attention_mask);

  std::vector<int> predictions;
  predictions.reserve(logits.size());

  for (const auto &token_logits : logits) {
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

} // namespace onnx_infer
