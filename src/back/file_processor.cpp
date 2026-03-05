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
 * @brief Переводит английскую метку типа обстоятельства на русский язык
 *
 * Автоматически удаляет префиксы B- и I- (BIO разметка) перед переводом.
 *
 * @param label Английская метка (например, "B-TIME", "I-MANNER")
 * @return std::string Русскоязычное название типа обстоятельства
 */
std::string FileProcessor::translate_label(const std::string &label) {
  // Убираем префикс B- или I-
  std::string clean_label = label;
  if (label.size() > 2 &&
      (label.substr(0, 2) == "B-" || label.substr(0, 2) == "I-")) {
    clean_label = label.substr(2);
  }

  auto it = label_translations_.find(clean_label);
  if (it != label_translations_.end()) {
    return it->second;
  }
  return clean_label;
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
 * @brief Форматирует предложение, выделяя обстоятельства квадратными скобками
 *
 * Вставляет открывающую скобку [ перед началом обстоятельства
 * и закрывающую скобку ] после его окончания.
 *
 * @param sentence Исходное предложение
 * @param entities Вектор найденных обстоятельств с позициями
 * @return std::string Предложение с выделенными обстоятельствами
 */
std::string FileProcessor::format_sentence_with_entities(
    const std::string &sentence,
    const std::vector<BertOnnxInference::Entity> &entities) {

  if (entities.empty()) {
    return sentence;
  }

  // Создаем копию предложения
  std::string formatted = sentence;

  // Вставляем скобки вокруг обстоятельств
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

    // Извлекаем обстоятельства
    auto start_time = std::chrono::high_resolution_clock::now();
    auto sentences_results = detector_.extract_circumstances(content);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    // Сохраняем результаты
    result.sentences = sentences_results;
    result.total_sentences = sentences_results.size();

    // Считаем общее количество обстоятельств
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

    // Считаем по типам обстоятельств
    for (const auto &sentence_result : file_result.sentences) {
      for (const auto &entity : sentence_result.entities) {
        std::string type = entity.type;
        stats.entity_type_counts[type]++;

        // Добавляем предложение для этого обстоятельства
        std::string entity_key =
            entity.text + " (" + translate_label(type) + ")";
        stats.entity_to_sentences[entity_key].insert(sentence_result.text);
      }
    }
  }

  return stats;
}

/**
 * @brief Записывает файл sentences.txt с детальными результатами
 *
 * Содержит для каждого файла все предложения с выделенными обстоятельствами
 * и списком найденных обстоятельств.
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

  for (const auto &file_result : all_results) {
    // Заголовок файла
    out_file << "=== Файл: " << file_result.filename << " ===\n";
    out_file << "=== Предложений: " << file_result.total_sentences
             << ", Обстоятельств: " << file_result.total_entities << " ===\n\n";

    // Предложения с обстоятельствами
    for (const auto &sentence_result : file_result.sentences) {
      // Предложение с выделением обстоятельств в квадратных скобках
      std::string formatted_sentence = format_sentence_with_entities(
          sentence_result.text, sentence_result.entities);

      out_file << formatted_sentence << "\n";

      // Список обстоятельств
      if (!sentence_result.entities.empty()) {
        for (const auto &entity : sentence_result.entities) {
          out_file << "- " << entity.text << " ("
                   << translate_label(entity.type) << ")\n";
        }
        out_file << "\n";
      } else {
        out_file << "(нет обстоятельств)\n\n";
      }
    }

    out_file << "\n";
  }

  out_file.close();
}

/**
 * @brief Записывает файл statistics.txt с общей статистикой
 *
 * Содержит:
 * - Общую статистику по файлам, предложениям, обстоятельствам
 * - Распределение обстоятельств по типам
 * - Статистику по каждому файлу
 * - Все найденные обстоятельства с примерами предложений
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

  out_file << "СТАТИСТИКА ОБРАБОТКИ\n";
  out_file << "===================\n\n";

  out_file << "ОБЩАЯ СТАТИСТИКА:\n";
  out_file << "• Обработано файлов: " << stats.total_files << "\n";
  out_file << "• Всего предложений: " << stats.total_sentences << "\n";
  out_file << "• Всего обстоятельств: " << stats.total_entities << "\n";

  if (stats.total_files > 0) {
    out_file << "• Среднее на файл: "
             << static_cast<float>(stats.total_entities) / stats.total_files
             << " обстоятельств\n";
  }

  if (stats.total_sentences > 0) {
    out_file << "• Среднее на предложение: "
             << static_cast<float>(stats.total_entities) / stats.total_sentences
             << " обстоятельств\n";
  }

  out_file << "\nОБСТОЯТЕЛЬСТВА ПО ТИПАМ:\n";
  out_file << "------------------------\n";
  for (const auto &[type, count] : stats.entity_type_counts) {
    std::string translated = translate_label(type);
    float percentage = stats.total_entities > 0 ? static_cast<float>(count) /
                                                      stats.total_entities * 100
                                                : 0;

    out_file << "• " << translated << " (" << type << "): " << count << " ("
             << std::fixed << std::setprecision(1) << percentage << "%)\n";
  }
  out_file << "\n";

  out_file << "ОБСТОЯТЕЛЬСТВА ПО ФАЙЛАМ:\n";
  out_file << "-------------------------\n";
  for (const auto &[filename, count] : stats.entities_per_file) {
    out_file << "• " << filename << ": " << count << " обстоятельств\n";
  }
  out_file << "\n";

  out_file << "ВСЕ НАЙДЕННЫЕ ОБСТОЯТЕЛЬСТВА:\n";
  out_file << "-----------------------------\n";
  for (const auto &[entity_key, sentences] : stats.entity_to_sentences) {
    out_file << "\n" << entity_key << ":\n";
    int example_count = 0;
    for (const auto &sentence : sentences) {
      if (example_count < 3) { // Показываем максимум 3 примера
        out_file << "  - \"" << sentence << "\"\n";
        example_count++;
      } else {
        out_file << "  - ... и еще " << (sentences.size() - 3)
                 << " предложений\n";
        break;
      }
    }
  }

  out_file.close();
}

/**
 * @brief Записывает все результаты обработки в выходную директорию
 *
 * Создает три файла:
 * - sentences.txt - детальные результаты с выделенными обстоятельствами
 * - statistics.txt - статистика по всем файлам
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

    // Дополнительно: сохраняем детальные результаты в JSON
    std::ofstream json_file(output_dir + "/results.json");
    if (json_file.is_open()) {
      json_file << "{\n";
      json_file << "  \"total_files\": " << stats.total_files << ",\n";
      json_file << "  \"total_sentences\": " << stats.total_sentences << ",\n";
      json_file << "  \"total_entities\": " << stats.total_entities << ",\n";
      json_file << "  \"files\": [\n";

      for (size_t i = 0; i < all_results.size(); i++) {
        const auto &file_result = all_results[i];
        json_file << "    {\n";
        json_file << "      \"filename\": \"" << file_result.filename
                  << "\",\n";
        json_file << "      \"sentences\": " << file_result.total_sentences
                  << ",\n";
        json_file << "      \"entities\": " << file_result.total_entities
                  << ",\n";
        json_file << "      \"sentences_list\": [\n";

        for (size_t j = 0; j < file_result.sentences.size(); j++) {
          const auto &sentence = file_result.sentences[j];
          json_file << "        {\n";
          json_file << "          \"text\": \"" << sentence.text << "\",\n";
          json_file << "          \"entities\": [\n";

          for (size_t k = 0; k < sentence.entities.size(); k++) {
            const auto &entity = sentence.entities[k];
            json_file << "            {\n";
            json_file << "              \"text\": \"" << entity.text << "\",\n";
            json_file << "              \"type\": \"" << entity.type << "\",\n";
            json_file << "              \"start\": " << entity.start << ",\n";
            json_file << "              \"end\": " << entity.end << "\n";
            json_file << "            }";

            if (k < sentence.entities.size() - 1) {
              json_file << ",";
            }
            json_file << "\n";
          }

          json_file << "          ]\n";
          json_file << "        }";

          if (j < file_result.sentences.size() - 1) {
            json_file << ",";
          }
          json_file << "\n";
        }

        json_file << "      ]\n";
        json_file << "    }";

        if (i < all_results.size() - 1) {
          json_file << ",";
        }
        json_file << "\n";
      }

      json_file << "  ]\n";
      json_file << "}\n";
      json_file.close();

      std::cout << "    ✓ Created: results.json\n";
    }

  } catch (const std::exception &e) {
    std::cerr << "    ✗ Error saving results: " << e.what() << "\n";
    throw;
  }
}
