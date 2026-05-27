// onnx_model.h
#ifndef ONNX_MODEL_H
#define ONNX_MODEL_H

#include <memory>
#ifdef WIN32
#include "../../onnxruntime/include/onnxruntime_cxx_api.h"
#else
#include <onnxruntime/onnxruntime_cxx_api.h>
#endif
#include <string>
#include <vector>

namespace onnx_infer {

/**
 * @brief Класс для работы с ONNX моделью BERT для NER
 *
 * Предоставляет методы для загрузки ONNX модели и выполнения инференса
 * для задач Named Entity Recognition (NER) с использованием BERT.
 */
class BertNerModel {
public:
  /**
   * @brief Конструктор
   * @param model_path Путь к ONNX модели
   *
   * Загружает ONNX модель по указанному пути и инициализирует сессию ONNX
   * Runtime. При ошибке загрузки выбрасывает исключение.
   */
  explicit BertNerModel(const std::string &model_path);

  /**
   * @brief Инференс модели
   * @param input_ids ID токенов (последовательность целых чисел)
   * @param attention_mask Маска внимания (1 для реальных токенов, 0 для
   * padding)
   * @return Логиты для каждого токена [seq_len, num_labels]
   *
   * Выполняет прямой проход по модели и возвращает сырые логиты для каждого
   * токена.
   */
  std::vector<std::vector<float>>
  predict(const std::vector<int64_t> &input_ids,
          const std::vector<int64_t> &attention_mask) const;

  /**
   * @brief Получение предсказанных меток
   * @param input_ids ID токенов (последовательность целых чисел)
   * @param attention_mask Маска внимания (1 для реальных токенов, 0 для
   * padding)
   * @return Метки для каждого токена (индексы классов с максимальной
   * вероятностью)
   *
   * Выполняет инференс и возвращает предсказанные классы для каждого токена,
   * выбирая класс с максимальным значением логита.
   */
  std::vector<int>
  predict_labels(const std::vector<int64_t> &input_ids,
                 const std::vector<int64_t> &attention_mask) const;

  /**
   * @brief Получить количество меток
   * @return Количество классов для NER (типы сущностей)
   */
  size_t num_labels() const;

  /**
   * @brief Получить максимальную длину последовательности
   * @return Максимальная длина последовательности, поддерживаемая моделью
   */
  size_t max_seq_length() const;

private:
  Ort::Env env_{ORT_LOGGING_LEVEL_WARNING, "BertNer"}; ///< окружение
  Ort::SessionOptions session_options_;   ///< Опции сессии ONNX Runtime
  std::unique_ptr<Ort::Session> session_; ///< Сессия ONNX модели

  size_t num_labels_ = 11;      ///< Количество меток NER
  size_t max_seq_length_ = 128; ///< Максимальная длина последовательности (по
                                ///< умолчанию 128, будет обновлено из модели)

  /// Имена входов ONNX модели
  const char *input_names_[2] = {"input_ids", "attention_mask"};

  /// Имена выходов ONNX модели
  const char *output_names_[1] = {"logits"};

  /**
   * @brief Создать ONNX тензор для input_ids
   * @param input_ids Вектор ID токенов
   * @return ONNX тензор для входных ID токенов
   *
   * Преобразует вектор целых чисел в формат ONNX тензора с размерностью [1,
   * seq_len].
   */
  Ort::Value
  create_input_ids_tensor(const std::vector<int64_t> &input_ids) const;

  /**
   * @brief Создать ONNX тензор для attention_mask
   * @param attention_mask Вектор маски внимания (int64)
   * @return ONNX тензор для маски внимания
   *
   * Преобразует маску внимания из int64 в float и создает ONNX тензор.
   */
  Ort::Value create_attention_mask_tensor(
      const std::vector<int64_t> &attention_mask) const;

  /**
   * @brief Создать все входные тензоры для модели
   * @param input_ids Вектор ID токенов
   * @param attention_mask Вектор маски внимания
   * @return Вектор из двух ONNX тензоров (input_ids, attention_mask)
   *
   * Утилитарный метод для создания полного набора входных тензоров.
   */
  std::vector<Ort::Value>
  create_input_tensors(const std::vector<int64_t> &input_ids,
                       const std::vector<int64_t> &attention_mask) const;

  /**
   * @brief Извлечь логиты из выходных тензоров
   * @param output_tensors Вектор выходных тензоров от ONNX Runtime
   * @return Двумерный вектор логитов [seq_len, num_labels]
   *
   * Преобразует сырые данные из ONNX тензора в удобный для использования
   * формат.
   */
  std::vector<std::vector<float>>
  extract_logits_from_output(std::vector<Ort::Value> &output_tensors) const;
};

} // namespace onnx_infer

#endif // ONNX_MODEL_H
