#ifndef BERT_ONNX_INFERENCE_H
#define BERT_ONNX_INFERENCE_H

#include "simple_tokenizer.h"
#include <map>
#include <memory>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <string>
#include <vector>

/**
 * @brief Класс для инференса BERT модели NER через ONNX Runtime
 *
 * Предоставляет функциональность для загрузки модели BERT,
 * токенизации текста и извлечения обстоятельств (NER).
 * Использует ONNX Runtime для выполнения модели.
 */
class BertOnnxInference {
public:
  /**
   * @brief Структура, представляющая найденную сущность (обстоятельство)
   */
  struct Entity {
    std::string text; // Текст сущности
    std::string type; // Тип сущности (TIME, MANNER, DEGREE и т.д.)
    size_t start;     // Начальная позиция в байтах
    size_t end;       // Конечная позиция в байтах
    std::vector<std::string> tokens; // Токены, составляющие сущность
  };

  /**
   * @brief Результат анализа одного предложения
   */
  struct SentenceResult {
    std::string text;             // Текст предложения
    std::vector<Entity> entities; // Найденные сущности
  };

  /**
   * @brief Конструктор класса BertOnnxInference
   *
   * @param model_path Путь к файлу ONNX модели
   * @param vocab_path Путь к файлу словаря токенизатора
   */
  BertOnnxInference(const std::string &model_path,
                    const std::string &vocab_path);

  /**
   * @brief Деструктор класса
   */
  ~BertOnnxInference();

  /**
   * @brief Извлекает обстоятельства из текста
   *
   * @param text Входной текст для анализа
   * @return std::vector<SentenceResult> Результаты анализа по предложениям
   */
  std::vector<SentenceResult> extract_circumstances(const std::string &text);

  /**
   * @brief Структура для сбора статистики обработки
   */
  struct Stats {
    size_t total_sentences = 0;                  // Всего обработано предложений
    size_t total_entities = 0;                   // Всего найдено сущностей
    std::map<std::string, size_t> entity_counts; // Счетчики по типам
  };

  /**
   * @brief Получает текущую статистику
   *
   * @return Stats Текущая статистика обработки
   */
  Stats get_stats() const { return stats_; }

private:
  Ort::Env env;                  // ONNX Runtime окружение
  Ort::Session session{nullptr}; // ONNX сессия для выполнения модели
  std::unique_ptr<SimpleTokenizer> tokenizer; // Токенизатор
  Stats stats_;                               // Внутренняя статистика

  /**
   * @brief Метки NER в формате BIO (должны совпадать с Python проектом)
   */
  static const std::vector<std::string> LABELS;

  /**
   * @brief Максимальная длина последовательности (из Python конфигурации)
   */
  static const size_t MAX_LEN = 128;

  /**
   * @brief Разделяет текст на предложения
   *
   * @param text Входной текст
   * @return std::vector<std::string> Вектор предложений
   */
  std::vector<std::string> split_into_sentences(const std::string &text);

  /**
   * @brief Группирует токены в сущности на основе BIO меток
   *
   * @param tokenization Результат токенизации
   * @param labels Предсказанные метки для каждого токена
   * @param original_text Исходный текст для извлечения текста сущностей
   * @return std::vector<Entity> Сгруппированные сущности
   */
  std::vector<Entity>
  group_entities(const SimpleTokenizer::EncodingResult &tokenization,
                 const std::vector<std::string> &labels,
                 const std::string &original_text);

  /**
   * @brief Выполняет инференс модели
   *
   * @param input_ids Вектор ID токенов
   * @param attention_mask Вектор маски внимания
   * @return std::vector<float> Вектор логитов
   */
  std::vector<float> run_inference(const std::vector<int64_t> &input_ids,
                                   const std::vector<int64_t> &attention_mask);

  /**
   * @brief Обновляет статистику найденными сущностями
   *
   * @param entities Вектор найденных сущностей
   */
  void update_stats(const std::vector<Entity> &entities);

  /**
   * @brief Валидирует загруженную модель
   *
   * Проверяет соответствие модели ожидаемым параметрам
   */
  void validate_model();
};

#endif
