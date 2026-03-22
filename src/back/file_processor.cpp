#include "file_processor.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

/**
 * @brief Конструктор класса FileProcessor
 *
 * @param detector Ссылка на объект BertOnnxInference для анализа текста
 */
FileProcessor::FileProcessor(BertOnnxInference &detector)
    : detector_(detector) {}

/**
 * @brief Читает текстовый файл с поддержкой UTF-8 BOM
 *
 * Автоматически определяет и удаляет UTF-8 BOM маркер (EF BB BF),
 * если он присутствует в начале файла.
 *
 * @param path Путь к файлу для чтения
 * @return std::string Содержимое файла в виде строки
 * @throws std::runtime_error Если не удается открыть файл
 */
std::string FileProcessor::read_text_file(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + path);
  }

  // Определяем размер файла
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Читаем данные
  std::vector<char> buffer(size);
  file.read(buffer.data(), size);
  file.close();

  // Проверяем BOM маркер UTF-8
  if (size >= 3 && static_cast<unsigned char>(buffer[0]) == 0xEF &&
      static_cast<unsigned char>(buffer[1]) == 0xBB &&
      static_cast<unsigned char>(buffer[2]) == 0xBF) {
    // Пропускаем BOM
    return std::string(buffer.begin() + 3, buffer.end());
  }

  return std::string(buffer.begin(), buffer.end());
}

/**
 * @brief Получает текущую дату и время в формате для имени директории
 *
 * @return std::string Строка с датой и временем в формате "YYYY-MM-DD_HH-MM-SS"
 */
std::string FileProcessor::get_current_datetime() {
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
  return ss.str();
}

/**
 * @brief Создает директорию для выходных файлов с уникальным именем
 *
 * Создает директорию вида "base_path/YYYY-MM-DD_HH-MM-SS".
 * Если такая директория уже существует, добавляет числовой суффикс.
 *
 * @param base_path Базовый путь для создания директории
 * @return std::string Полный путь к созданной директории
 * @throws std::runtime_error Если не удается создать директорию
 */
std::string
FileProcessor::create_output_directory(const std::string &base_path) {
  std::string datetime_str = get_current_datetime();
  std::string output_dir = base_path + "/" + datetime_str;

  try {
    if (fs::exists(output_dir)) {
      // Добавляем суффикс, если директория уже существует
      int suffix = 1;
      while (fs::exists(output_dir + "_" + std::to_string(suffix))) {
        suffix++;
      }
      output_dir = output_dir + "_" + std::to_string(suffix);
    }

    fs::create_directories(output_dir);
    std::cout << "  Output directory: " << output_dir << "\n";

  } catch (const fs::filesystem_error &e) {
    throw std::runtime_error("Cannot create output directory: " +
                             std::string(e.what()));
  }

  return output_dir;
}

/**
 * @brief Форматирует предложение, выделяя члены предложения квадратными
 * скобками
 *
 * Вставляет открывающую скобку [ перед началом сущности
 * и закрывающую скобку ] после её окончания.
 *
 * @param sentence Исходное предложение
 * @param entities Вектор найденных сущностей с позициями
 * @return std::string Предложение с выделенными членами предложения
 */
std::string FileProcessor::format_sentence_with_entities(
    const std::string &sentence,
    const std::vector<BertOnnxInference::Entity> &entities) {

  if (entities.empty()) {
    return sentence;
  }

  // Создаем копию предложения
  std::string formatted = sentence;

  // Вставляем скобки вокруг сущностей
  // Начинаем с конца, чтобы индексы не сдвигались
  for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
    const auto &entity = *it;

    // Проверяем границы
    if (entity.start < formatted.length() && entity.end <= formatted.length()) {
      // Вставляем закрывающую скобку
      if (entity.end <= formatted.length()) {
        formatted.insert(entity.end, "]");
      }

      // Вставляем открывающую скобку
      if (entity.start < formatted.length()) {
        formatted.insert(entity.start, "[");
      }
    }
  }

  return formatted;
}

/**
 * @brief Обрабатывает один файл: читает, анализирует и возвращает результаты
 *
 * @param input_path Путь к входному файлу
 * @return FileResult Структура с результатами анализа файла
 */
FileProcessor::FileResult
FileProcessor::process_file(const std::string &input_path) {
  FileResult result;
  result.filename = fs::path(input_path).filename().string();

  std::cout << "  Processing: " << result.filename << "\n";

  try {
    // Читаем файл
    std::string content = read_text_file(input_path);

    if (content.empty()) {
      std::cout << "    ⚠ File is empty\n";
      return result;
    }

    // Извлекаем члены предложения
    auto start_time = std::chrono::high_resolution_clock::now();
    auto sentences_results = detector_.extract_sentence_parts(content);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    // Сохраняем результаты
    result.sentences = sentences_results;
    result.total_sentences = sentences_results.size();

    // Считаем общее количество сущностей
    for (const auto &sentence_result : sentences_results) {
      result.total_entities += sentence_result.entities.size();
    }

    std::cout << "    ✓ Found " << result.total_sentences << " sentences, "
              << result.total_entities << " entities (" << duration.count()
              << " ms)\n";

  } catch (const std::exception &e) {
    std::cerr << "    ✗ Error processing file: " << e.what() << "\n";
  }

  return result;
}

/**
 * @brief Обрабатывает все текстовые файлы в указанной директории
 *
 * Находит все файлы с расширениями .txt и .text в директории,
 * обрабатывает каждый файл и сохраняет результаты.
 *
 * @param input_dir Путь к директории с входными файлами
 * @param output_base Базовый путь для сохранения результатов
 * @throws std::runtime_error Если не удается прочитать директорию
 */
void FileProcessor::process_directory(const std::string &input_dir,
                                      const std::string &output_base) {
  std::vector<FileResult> all_results;

  // Собираем все текстовые файлы
  std::vector<fs::path> text_files;

  try {
    for (const auto &entry : fs::directory_iterator(input_dir)) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();

        // Приводим расширение к нижнему регистру
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (ext == ".txt" || ext == ".text") {
          text_files.push_back(entry.path());
        }
      }
    }
  } catch (const fs::filesystem_error &e) {
    throw std::runtime_error("Cannot read directory: " + std::string(e.what()));
  }

  if (text_files.empty()) {
    std::cout << "  No text files found in directory: " << input_dir << "\n";
    return;
  }

  std::cout << "  Found " << text_files.size() << " text file(s)\n\n";

  // Обрабатываем файлы
  for (const auto &file_path : text_files) {
    FileResult result = process_file(file_path.string());
    if (result.total_sentences > 0) {
      all_results.push_back(result);
    }
  }

  if (all_results.empty()) {
    std::cout << "\n  No valid results to save.\n";
    return;
  }

  // Создаем выходную директорию
  std::string output_dir = create_output_directory(output_base);

  // Рассчитываем статистику
  Statistics stats = calculate_statistics(all_results);

  // Записываем результаты
  write_results(output_dir, all_results, stats);

  std::cout << "\n  Results saved to: " << output_dir << "\n";
}

/**
 * @brief Рассчитывает статистику по всем обработанным файлам
 *
 * @param all_results Вектор результатов обработки файлов
 * @return Statistics Структура с собранной статистикой
 */
FileProcessor::Statistics FileProcessor::calculate_statistics(
    const std::vector<FileResult> &all_results) {
  Statistics stats;
  stats.total_files = all_results.size();

  for (const auto &file_result : all_results) {
    stats.total_sentences += file_result.total_sentences;
    stats.total_entities += file_result.total_entities;
    stats.entities_per_file[file_result.filename] = file_result.total_entities;

    // Считаем по типам членов предложения
    for (const auto &sentence_result : file_result.sentences) {
      for (const auto &entity : sentence_result.entities) {
        std::string type = entity.type;
        stats.entity_type_counts[type]++;

        std::string type_ru = entity.type_ru;
        stats.entity_type_counts_ru[type_ru]++;

        // Добавляем предложение для этой сущности
        std::string entity_key = entity.text + " (" + type_ru + ")";
        stats.entity_to_sentences[entity_key].insert(sentence_result.text);
      }
    }
  }

  return stats;
}

/**
 * @brief Записывает файл sentences.txt с детальными результатами
 *
 * Содержит для каждого файла все предложения с выделенными членами предложения
 * и списком найденных сущностей.
 *
 * @param path Путь для сохранения файла
 * @param all_results Вектор результатов обработки файлов
 * @param stats Статистика обработки
 * @throws std::runtime_error Если не удается создать файл
 */
void FileProcessor::write_sentences_file(
    const std::string &path, const std::vector<FileResult> &all_results,
    const Statistics &stats) {
  std::ofstream out_file(path + "/sentences.txt",
                         std::ios::out | std::ios::binary);

  if (!out_file.is_open()) {
    throw std::runtime_error("Cannot create sentences.txt");
  }

  // UTF-8 BOM для совместимости
  const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
  out_file.write(reinterpret_cast<const char *>(bom), sizeof(bom));

  out_file << "=== АНАЛИЗ ЧЛЕНОВ ПРЕДЛОЖЕНИЯ ===\n\n";
  out_file << "Всего файлов: " << stats.total_files << "\n";
  out_file << "Всего предложений: " << stats.total_sentences << "\n";
  out_file << "Всего членов предложения: " << stats.total_entities << "\n\n";

  for (const auto &file_result : all_results) {
    // Заголовок файла
    out_file << "========================================\n";
    out_file << "ФАЙЛ: " << file_result.filename << "\n";
    out_file << "========================================\n";
    out_file << "Предложений: " << file_result.total_sentences
             << ", Членов предложения: " << file_result.total_entities
             << "\n\n";

    // Предложения с выделенными членами предложения
    int sent_num = 1;
    for (const auto &sentence_result : file_result.sentences) {
      // Предложение с выделением в квадратных скобках
      std::string formatted_sentence = format_sentence_with_entities(
          sentence_result.text, sentence_result.entities);

      out_file << "[" << sent_num << "] " << formatted_sentence << "\n";

      // Список членов предложения
      if (!sentence_result.entities.empty()) {
        // Группируем по типам для лучшей читаемости
        std::map<std::string, std::vector<std::string>> entities_by_type;

        for (const auto &entity : sentence_result.entities) {
          entities_by_type[entity.type_ru].push_back(entity.text);
        }

        for (const auto &[type, texts] : entities_by_type) {
          out_file << "   " << type << ": ";
          for (size_t i = 0; i < texts.size(); i++) {
            out_file << "\"" << texts[i] << "\"";
            if (i < texts.size() - 1) {
              out_file << ", ";
            }
          }
          out_file << "\n";
        }
      } else {
        out_file << "   (нет членов предложения)\n";
      }
      out_file << "\n";
      sent_num++;
    }

    out_file << "\n";
  }

  out_file.close();
}

/**
 * @brief Записывает файл statistics.txt с общей статистикой
 *
 * Содержит:
 * - Общую статистику по файлам, предложениям, сущностям
 * - Распределение членов предложения по типам
 * - Статистику по каждому файлу
 * - Все найденные члены предложения с примерами предложений
 *
 * @param path Путь для сохранения файла
 * @param stats Статистика обработки
 * @throws std::runtime_error Если не удается создать файл
 */
void FileProcessor::write_statistics_file(const std::string &path,
                                          const Statistics &stats) {
  std::ofstream out_file(path + "/statistics.txt",
                         std::ios::out | std::ios::binary);

  if (!out_file.is_open()) {
    throw std::runtime_error("Cannot create statistics.txt");
  }

  // UTF-8 BOM
  const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
  out_file.write(reinterpret_cast<const char *>(bom), sizeof(bom));

  out_file << "СТАТИСТИКА АНАЛИЗА ЧЛЕНОВ ПРЕДЛОЖЕНИЯ\n";
  out_file << "======================================\n\n";

  // ОБЩАЯ СТАТИСТИКА
  out_file << "ОБЩАЯ СТАТИСТИКА:\n";
  out_file << "-----------------\n";
  out_file << "• Обработано файлов: " << stats.total_files << "\n";
  out_file << "• Всего предложений: " << stats.total_sentences << "\n";
  out_file << "• Всего членов предложения: " << stats.total_entities << "\n";

  if (stats.total_files > 0) {
    out_file << "• Среднее на файл: "
             << static_cast<float>(stats.total_entities) / stats.total_files
             << " членов предложения\n";
  }

  if (stats.total_sentences > 0) {
    out_file << "• Среднее на предложение: "
             << static_cast<float>(stats.total_entities) / stats.total_sentences
             << " членов предложения\n";
  }

  out_file << "\n";

  // РАСПРЕДЕЛЕНИЕ ПО ТИПАМ
  out_file << "РАСПРЕДЕЛЕНИЕ ПО ТИПАМ:\n";
  out_file << "----------------------\n";

  // Сортируем по убыванию
  std::vector<std::pair<std::string, int>> sorted_types;
  for (const auto &[type, count] : stats.entity_type_counts_ru) {
    sorted_types.push_back({type, count});
  }

  std::sort(sorted_types.begin(), sorted_types.end(),
            [](const auto &a, const auto &b) { return a.second > b.second; });

  for (const auto &[type, count] : sorted_types) {
    float percentage = stats.total_entities > 0 ? static_cast<float>(count) /
                                                      stats.total_entities * 100
                                                : 0;
    out_file << "• " << type << ": " << count << " (" << std::fixed
             << std::setprecision(1) << percentage << "%)\n";
  }
  out_file << "\n";

  // СТАТИСТИКА ПО ФАЙЛАМ
  out_file << "СТАТИСТИКА ПО ФАЙЛАМ:\n";
  out_file << "--------------------\n";
  for (const auto &[filename, count] : stats.entities_per_file) {
    out_file << "• " << filename << ": " << count << " членов предложения\n";
  }
  out_file << "\n";

  // ВСЕ НАЙДЕННЫЕ ЧЛЕНЫ ПРЕДЛОЖЕНИЯ
  out_file << "ВСЕ НАЙДЕННЫЕ ЧЛЕНЫ ПРЕДЛОЖЕНИЯ:\n";
  out_file << "--------------------------------\n";

  // Сортируем по количеству вхождений
  std::vector<std::pair<std::string, std::set<std::string>>> sorted_entities(
      stats.entity_to_sentences.begin(), stats.entity_to_sentences.end());

  std::sort(sorted_entities.begin(), sorted_entities.end(),
            [](const auto &a, const auto &b) {
              return a.second.size() > b.second.size();
            });

  for (const auto &[entity_key, sentences] : sorted_entities) {
    out_file << "\n"
             << entity_key << " (встречается " << sentences.size()
             << " раз):\n";

    int example_count = 0;
    for (const auto &sentence : sentences) {
      if (example_count < 3) { // Показываем максимум 3 примера
        out_file << "  • \"" << sentence << "\"\n";
        example_count++;
      } else {
        out_file << "  • ... и еще " << (sentences.size() - 3)
                 << " предложений\n";
        break;
      }
    }
  }

  out_file.close();
}

/**
 * @brief Записывает файл summary.txt с краткой сводкой
 *
 * @param path Путь для сохранения файла
 * @param all_results Вектор результатов обработки файлов
 * @param stats Статистика обработки
 */
void FileProcessor::write_summary_file(
    const std::string &path, const std::vector<FileResult> &all_results,
    const Statistics &stats) {
  std::ofstream out_file(path + "/summary.txt",
                         std::ios::out | std::ios::binary);

  if (!out_file.is_open()) {
    return; // Не критично, если не удалось создать
  }

  // UTF-8 BOM
  const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
  out_file.write(reinterpret_cast<const char *>(bom), sizeof(bom));

  out_file << "КРАТКАЯ СВОДКА АНАЛИЗА\n";
  out_file << "======================\n\n";

  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  out_file << "Дата анализа: "
           << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
           << "\n\n";

  out_file << "ИТОГИ:\n";
  out_file << "• Файлов обработано: " << stats.total_files << "\n";
  out_file << "• Предложений обработано: " << stats.total_sentences << "\n";
  out_file << "• Членов предложения найдено: " << stats.total_entities
           << "\n\n";

  // Топ-5 самых частых членов предложения
  if (!stats.entity_to_sentences.empty()) {
    out_file << "САМЫЕ ЧАСТЫЕ ЧЛЕНЫ ПРЕДЛОЖЕНИЯ:\n";

    std::vector<std::pair<std::string, size_t>> entity_counts;
    for (const auto &[entity_key, sentences] : stats.entity_to_sentences) {
      entity_counts.push_back({entity_key, sentences.size()});
    }

    std::sort(entity_counts.begin(), entity_counts.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    int top_count = std::min(5, (int)entity_counts.size());
    for (int i = 0; i < top_count; i++) {
      out_file << (i + 1) << ". " << entity_counts[i].first << " ("
               << entity_counts[i].second << " раз)\n";
    }
  }

  out_file.close();
}

/**
 * @brief Записывает все результаты обработки в выходную директорию
 *
 * Создает четыре файла:
 * - sentences.txt - детальные результаты с выделенными членами предложения
 * - statistics.txt - статистика по всем файлам
 * - summary.txt - краткая сводка анализа
 * - results.json - структурированные результаты в формате JSON
 *
 * @param output_dir Директория для сохранения результатов
 * @param all_results Вектор результатов обработки файлов
 * @param stats Статистика обработки
 * @throws std::runtime_error Если не удается сохранить результаты
 */
void FileProcessor::write_results(const std::string &output_dir,
                                  const std::vector<FileResult> &all_results,
                                  const Statistics &stats) {
  std::cout << "\n  Saving results...\n";

  try {
    // Записываем sentences.txt
    write_sentences_file(output_dir, all_results, stats);
    std::cout << "    ✓ Created: sentences.txt\n";

    // Записываем statistics.txt
    write_statistics_file(output_dir, stats);
    std::cout << "    ✓ Created: statistics.txt\n";

    // Записываем summary.txt
    write_summary_file(output_dir, all_results, stats);
    std::cout << "    ✓ Created: summary.txt\n";

  } catch (const std::exception &e) {
    std::cerr << "    ✗ Error saving results: " << e.what() << "\n";
    throw;
  }
}
