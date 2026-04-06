// onnx_model.h
#ifndef ONNX_MODEL_H
#define ONNX_MODEL_H

#include <memory>
#ifdef WIN32
#include "../../onnxruntime/include/onnxruntime/onnxruntime_cxx_api.h"
#else
#include <onnxruntime/onnxruntime_cxx_api.h>
#endif
#include <string>
#include <vector>

namespace onnx_infer {

/**
 * @brief Класс для работы с ONNX моделью BERT для NER
 */
class BertNerModel {
public:
  /**
   * @brief Конструктор
   * @param model_path Путь к ONNX модели
   */
  explicit BertNerModel(const std::string &model_path);

  /**
   * @brief Инференс модели
   * @param input_ids ID токенов
   * @param attention_mask Маска внимания
   * @return Логиты для каждого токена [seq_len, num_labels]
   */
  std::vector<std::vector<float>>
  predict(const std::vector<int64_t> &input_ids,
          const std::vector<int64_t> &attention_mask) const;

  /**
   * @brief Получение предсказанных меток
   * @param input_ids ID токенов
   * @param attention_mask Маска внимания
   * @return Метки для каждого токена
   */
  std::vector<int>
  predict_labels(const std::vector<int64_t> &input_ids,
                 const std::vector<int64_t> &attention_mask) const;

  /**
   * @brief Получить количество меток
   */
  size_t num_labels() const { return num_labels_; }

  /**
   * @brief Получить максимальную длину последовательности
   */
  size_t max_seq_length() const { return max_seq_length_; }

private:
  Ort::Env env_{ORT_LOGGING_LEVEL_WARNING, "BertNer"};
  Ort::SessionOptions session_options_;
  std::unique_ptr<Ort::Session> session_;

  size_t num_labels_ = 11;      // По умолчанию, будет обновлено из модели
  size_t max_seq_length_ = 128; // По умолчанию, будет обновлено из модели

  // Имена входов/выходов ONNX модели
  const char *input_names_[2] = {"input_ids", "attention_mask"};
  const char *output_names_[1] = {"logits"};

  /**
   * @brief Получить информацию о модели
   */
  void get_model_info();
};

} // namespace onnx_infer

#endif // ONNX_MODEL_H
