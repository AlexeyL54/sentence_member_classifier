#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include "bert_onnx_inference.h"
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * @brief Класс для пакетной обработки файлов
 *
 * Предоставляет функциональность для обработки нескольких текстовых файлов,
 * сохранения результатов и сбора статистики.
 */
class FileProcessor {
public:
  /**
   * @brief Результат обработки одного файла
   */
  struct FileResult {
    std::string filename; // Имя файла
    std::vector<BertOnnxInference::SentenceResult>
        sentences;           // Результаты по предложениям
    int total_entities = 0;  // Всего сущностей в файле
    int total_sentences = 0; // Всего предложений в файле
  };

  /**
   * @brief Статистика по всем обработанным файлам
   */
  struct Statistics {
    int total_files = 0;                           // Всего файлов
    int total_sentences = 0;                       // Всего предложений
    int total_entities = 0;                        // Всего сущностей
    std::map<std::string, int> entity_type_counts; // Счетчики по типам
    std::map<std::string, std::set<std::string>>
        entity_to_sentences;                      // Сущность -> предложения
    std::map<std::string, int> entities_per_file; // Сущности по файлам
  };

  /**
   * @brief Конструктор класса FileProcessor
   *
   * @param detector Ссылка на объект детектора для анализа
   */
  FileProcessor(BertOnnxInference &detector);

  /**
   * @brief Обрабатывает все текстовые файлы в директории
   *
   * @param input_dir Путь к директории с входными файлами
   * @param output_base Базовый путь для сохранения результатов
   */
  void process_directory(const std::string &input_dir,
                         const std::string &output_base);

  /**
   * @brief Обрабатывает один файл
   *
   * @param input_path Путь к входному файлу
   * @return FileResult Результаты обработки файла
   */
  FileResult process_file(const std::string &input_path);

  /**
   * @brief Получает текущую дату и время для имени директории
   *
   * @return std::string Строка с датой и временем
   */
  std::string get_current_datetime();

  /**
   * @brief Создает выходную директорию с уникальным именем
   *
   * @param base_path Базовый путь для создания
   * @return std::string Полный путь к созданной директории
   */
  std::string create_output_directory(const std::string &base_path);

  /**
   * @brief Записывает результаты обработки в файлы
   *
   * @param output_dir Директория для сохранения
   * @param all_results Вектор результатов по файлам
   * @param stats Статистика обработки
   */
  void write_results(const std::string &output_dir,
                     const std::vector<FileResult> &all_results,
                     const Statistics &stats);

private:
  BertOnnxInference &detector_; // Ссылка на детектор для анализа текста

  /**
   * @brief Словарь для перевода английских меток на русский язык
   */
  std::map<std::string, std::string> label_translations_ = {
      {"TIME", "обстоятельство времени"},
      {"MANNER", "обстоятельство образа действия"},
      {"DEGREE", "обстоятельство степени"},
      {"CONDITION", "обстоятельство условия"},
      {"CAUSE", "обстоятельство причины"},
      {"CONCESSION", "обстоятельство уступки"},
      {"LOCATION", "обстоятельство места"},
      {"PURPOSE", "обстоятельство цели"}};

  /**
   * @brief Рассчитывает статистику по результатам обработки
   *
   * @param all_results Вектор результатов по файлам
   * @return Statistics Рассчитанная статистика
   */
  Statistics calculate_statistics(const std::vector<FileResult> &all_results);

  /**
   * @brief Записывает файл sentences.txt с детальными результатами
   *
   * @param path Путь для сохранения
   * @param all_results Результаты по файлам
   * @param stats Статистика обработки
   */
  void write_sentences_file(const std::string &path,
                            const std::vector<FileResult> &all_results,
                            const Statistics &stats);

  /**
   * @brief Записывает файл statistics.txt со статистикой
   *
   * @param path Путь для сохранения
   * @param stats Статистика обработки
   */
  void write_statistics_file(const std::string &path, const Statistics &stats);

  /**
   * @brief Форматирует предложение с выделением обстоятельств
   *
   * @param sentence Исходное предложение
   * @param entities Найденные сущности
   * @return std::string Предложение с обстоятельствами в квадратных скобках
   */
  std::string format_sentence_with_entities(
      const std::string &sentence,
      const std::vector<BertOnnxInference::Entity> &entities);

  /**
   * @brief Переводит английскую метку на русский язык
   *
   * @param label Английская метка
   * @return std::string Русский перевод
   */
  std::string translate_label(const std::string &label);

  /**
   * @brief Читает текстовый файл с поддержкой UTF-8 BOM
   *
   * @param path Путь к файлу
   * @return std::string Содержимое файла
   */
  std::string read_text_file(const std::string &path);
};

#endif
