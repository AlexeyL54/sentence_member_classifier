#include "src/back/bert_onnx_inference.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

/**
 * @brief Проверяет и создает необходимые директории для работы программы
 *
 * Создает директории:
 * - text/ - для входных текстовых файлов
 * - outputs/ - для выходных результатов
 * - model/ - для файлов модели и словаря
 */
void ensure_directories() {
  std::vector<std::string> dirs = {"text", "outputs", "model"};

  for (const auto &dir : dirs) {
    if (!fs::exists(dir)) {
      try {
        fs::create_directory(dir);
        std::cout << "Создана директория: " << dir << std::endl;
      } catch (const fs::filesystem_error &e) {
        std::cerr << "Ошибка при создании директории " << dir << ": "
                  << e.what() << std::endl;
      }
    }
  }
}

/**
 * @brief Получает список текстовых файлов в директории text
 *
 * Ищет файлы с расширениями .txt и .text
 *
 * @return std::vector<std::string> Вектор имен файлов, отсортированный по
 * алфавиту
 */
std::vector<std::string> get_text_files() {
  std::vector<std::string> files;

  try {
    for (const auto &entry : fs::directory_iterator("text")) {
      if (entry.is_regular_file()) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (ext == ".txt" || ext == ".text") {
          files.push_back(entry.path().filename().string());
        }
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Ошибка при чтении директории text: " << e.what() << std::endl;
  }

  // Сортируем файлы для удобства выбора
  std::sort(files.begin(), files.end());

  return files;
}

/**
 * @brief Читает текстовый файл с поддержкой UTF-8 BOM
 *
 * Автоматически определяет и пропускает UTF-8 BOM маркер (EF BB BF)
 *
 * @param path Путь к файлу
 * @return std::string Содержимое файла в виде строки
 * @throws std::runtime_error Если не удается открыть файл
 */
std::string read_text_file(const std::string &path) {
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
 * @brief Переводит английское название типа обстоятельства на русский язык
 *
 * @param label Английское название типа (TIME, MANNER, DEGREE и т.д.)
 * @return std::string Русскоязычное название типа обстоятельства
 */
std::string translate_label(const std::string &label) {
  if (label == "TIME")
    return "обстоятельство времени";
  if (label == "MANNER")
    return "обстоятельство образа действия";
  if (label == "DEGREE")
    return "обстоятельство степени";
  if (label == "CONDITION")
    return "обстоятельство условия";
  if (label == "CAUSE")
    return "обстоятельство причины";
  if (label == "CONCESSION")
    return "обстоятельство уступки";
  if (label == "LOCATION")
    return "обстоятельство места";
  if (label == "PURPOSE")
    return "обстоятельство цели";
  return label;
}

/**
 * @brief Отображает прогресс обработки в виде процентов
 *
 * @param current Текущее количество обработанных элементов
 * @param total Общее количество элементов для обработки
 */
void show_progress(int current, int total) {
  if (total == 0)
    return;

  int percentage = (current * 100) / total;
  std::cout << "\rОбработано: " << current << "/" << total << " предложений ("
            << percentage << "%)";
  std::cout.flush();

  if (current >= total) {
    std::cout << std::endl;
  }
}

/**
 * @brief Отображает анимированный спиннер во время длительных операций
 *
 * Запускается в отдельном потоке и отображает вращающийся символ
 * для индикации процесса обработки.
 *
 * @param stop_flag Флаг для остановки анимации
 * @param message Сообщение, отображаемое рядом со спиннером
 */
void show_processing_spinner(bool &stop_flag, const std::string &message) {
  const char spinner[] = {'|', '/', '-', '\\'};
  int spinner_index = 0;

  while (!stop_flag) {
    std::cout << "\r" << spinner[spinner_index % 4] << " " << message;
    std::cout.flush();
    spinner_index++;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  std::cout << "\r✓ " << message << " завершено!        " << std::endl;
}

/**
 * @brief Сохраняет результаты анализа в файлы
 *
 * Создает три файла в выходной директории:
 * - statistics.txt - подробная статистика по всем обстоятельствам
 * - sentences.txt - результаты по каждому предложению
 * - summary.txt - краткая сводка анализа
 *
 * @param output_dir Директория для сохранения результатов
 * @param filename Имя исходного файла
 * @param sentences Результаты анализа по предложениям
 * @param total_entities Общее количество найденных обстоятельств
 */
void save_results(
    const std::string &output_dir, const std::string &filename,
    const std::vector<BertOnnxInference::SentenceResult> &sentences,
    int total_entities) {

  std::cout << "Сохранение результатов..." << std::endl;

  // Собираем статистику
  std::map<std::string, std::pair<int, std::vector<int>>> unique_circumstances;
  std::map<std::string, int> type_counts;
  int sentence_index = 1;

  for (const auto &sentence_result : sentences) {
    for (const auto &entity : sentence_result.entities) {
      std::string key = entity.text;
      unique_circumstances[key].first++;
      unique_circumstances[key].second.push_back(sentence_index);
      type_counts[entity.type]++;
    }
    sentence_index++;
  }

  // 1. Сохраняем statistics.txt в новом формате
  std::ofstream stats_file(output_dir + "/statistics.txt", std::ios::binary);
  if (stats_file.is_open()) {
    // UTF-8 BOM
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    stats_file.write(reinterpret_cast<const char *>(bom), sizeof(bom));

    stats_file << "=== СТАТИСТИКА ОБРАБОТКИ ===" << std::endl << std::endl;

    // Общая статистика
    stats_file << "ОБЩАЯ СТАТИСТИКА:" << std::endl;
    stats_file << "• Обработан файл: " << filename << std::endl;
    stats_file << "• Всего предложений: " << sentences.size() << std::endl;
    stats_file << "• Уникальных обстоятельств: " << unique_circumstances.size()
               << std::endl;
    stats_file << "• Всего появлений обстоятельств: " << total_entities
               << std::endl
               << std::endl;

    // Статистика по типам
    stats_file << "РАСПРЕДЕЛЕНИЕ ПО ТИПАМ:" << std::endl;
    for (const auto &[type, count] : type_counts) {
      float percentage =
          total_entities > 0 ? (float)count / total_entities * 100 : 0;
      stats_file << "• " << translate_label(type) << ": " << count << " ("
                 << std::fixed << std::setprecision(1) << percentage << "%)"
                 << std::endl;
    }

    stats_file << std::endl
               << "=== ДЕТАЛЬНАЯ СТАТИСТИКА ОБСТОЯТЕЛЬСТВ ===" << std::endl
               << std::endl;

    // Детальная статистика (как в adverbial_list.txt)
    for (const auto &[circumstance, data] : unique_circumstances) {
      int count = data.first;
      const auto &sentence_numbers = data.second;

      stats_file << "Обстоятельство: \"" << circumstance << "\"" << std::endl;

      // Определяем тип обстоятельства (берем первый попавшийся)
      std::string circumstance_type = "неизвестный тип";
      for (const auto &sentence_result : sentences) {
        for (const auto &entity : sentence_result.entities) {
          if (entity.text == circumstance) {
            circumstance_type = translate_label(entity.type);
            break;
          }
        }
        if (circumstance_type != "неизвестный тип")
          break;
      }

      stats_file << "  Тип: " << circumstance_type << std::endl;
      stats_file << "  Количество появлений: " << count << std::endl;
      stats_file << "  Встречается в предложениях: ";

      for (size_t i = 0; i < sentence_numbers.size(); i++) {
        stats_file << sentence_numbers[i];
        if (i < sentence_numbers.size() - 1) {
          stats_file << ", ";
        }
      }
      stats_file << std::endl << std::endl;
    }

    stats_file << "=== ИТОГИ ===" << std::endl;
    stats_file << "Всего уникальных обстоятельств: "
               << unique_circumstances.size() << std::endl;
    stats_file << "Всего появлений обстоятельств: " << total_entities
               << std::endl;

    stats_file.close();
    std::cout << "  ✓ statistics.txt - статистика обстоятельств" << std::endl;
  }

  // 2. Сохраняем sentences.txt
  std::ofstream sentences_file(output_dir + "/sentences.txt", std::ios::binary);
  if (sentences_file.is_open()) {
    // UTF-8 BOM
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    sentences_file.write(reinterpret_cast<const char *>(bom), sizeof(bom));

    sentences_file << "=== РЕЗУЛЬТАТЫ АНАЛИЗА ТЕКСТА ===" << std::endl
                   << std::endl;
    sentences_file << "Файл: " << filename << std::endl;
    sentences_file << "Всего предложений: " << sentences.size() << std::endl;
    sentences_file << "Всего обстоятельств: " << total_entities << std::endl
                   << std::endl;

    int sent_num = 1;
    for (const auto &sentence_result : sentences) {
      sentences_file << "[" << sent_num << "] " << sentence_result.text
                     << std::endl;

      if (!sentence_result.entities.empty()) {
        sentences_file << "  Найдено обстоятельств: "
                       << sentence_result.entities.size() << std::endl;
        for (const auto &entity : sentence_result.entities) {
          sentences_file << "  - \"" << entity.text << "\" ("
                         << translate_label(entity.type) << ")" << std::endl;
        }
      } else {
        sentences_file << "  (нет обстоятельств)" << std::endl;
      }
      sentences_file << std::endl;
      sent_num++;
    }

    sentences_file.close();
    std::cout << "  ✓ sentences.txt - детали по каждому предложению"
              << std::endl;
  }

  // 3. Сохраняем summary.txt (краткая сводка)
  std::ofstream summary_file(output_dir + "/summary.txt", std::ios::binary);
  if (summary_file.is_open()) {
    // UTF-8 BOM
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    summary_file.write(reinterpret_cast<const char *>(bom), sizeof(bom));

    summary_file << "КРАТКАЯ СВОДКА АНАЛИЗА" << std::endl;
    summary_file << "======================" << std::endl << std::endl;

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;

#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");

    summary_file << "Дата анализа: " << ss.str() << std::endl;
    summary_file << "Анализируемый файл: " << filename << std::endl
                 << std::endl;
    summary_file << "РЕЗУЛЬТАТЫ:" << std::endl;
    summary_file << "• Предложений обработано: " << sentences.size()
                 << std::endl;
    summary_file << "• Уникальных обстоятельств найдено: "
                 << unique_circumstances.size() << std::endl;
    summary_file << "• Всего появлений обстоятельств: " << total_entities
                 << std::endl
                 << std::endl;

    // Топ-5 самых частых обстоятельств
    if (!unique_circumstances.empty()) {
      summary_file << "САМЫЕ ЧАСТЫЕ ОБСТОЯТЕЛЬСТВА:" << std::endl;
      std::vector<std::pair<std::string, int>> sorted_circumstances;
      for (const auto &[circumstance, data] : unique_circumstances) {
        sorted_circumstances.emplace_back(circumstance, data.first);
      }

      std::sort(
          sorted_circumstances.begin(), sorted_circumstances.end(),
          [](const auto &a, const auto &b) { return a.second > b.second; });

      int top_count = std::min(5, (int)sorted_circumstances.size());
      for (int i = 0; i < top_count; i++) {
        summary_file << (i + 1) << ". \"" << sorted_circumstances[i].first
                     << "\" (" << sorted_circumstances[i].second << " раз)"
                     << std::endl;
      }
    }

    summary_file.close();
    std::cout << "  ✓ summary.txt - краткая сводка анализа" << std::endl;
  }

  std::cout << std::endl
            << "Результаты сохранены в директории: " << output_dir << std::endl;
}

/**
 * @brief Главная функция программы
 *
 * Оркестрирует весь процесс:
 * 1. Проверяет наличие необходимых директорий
 * 2. Получает список доступных текстовых файлов
 * 3. Запрашивает выбор файла у пользователя
 * 4. Инициализирует модель BERT для NER
 * 5. Выполняет анализ выбранного файла
 * 6. Сохраняет результаты в выходные файлы
 *
 * @return int Код возврата (0 - успех, 1 - ошибка)
 */
int main() {
  std::cout << "=========================================" << std::endl;
  std::cout << "  Обнаружение обстоятельств в тексте" << std::endl;
  std::cout << "  Использование ONNX модели BERT для NER" << std::endl;
  std::cout << "=========================================" << std::endl
            << std::endl;

  // Проверяем и создаем необходимые директории
  ensure_directories();

  // Получаем список доступных файлов
  auto text_files = get_text_files();

  if (text_files.empty()) {
    std::cout << "В директории 'text' не найдено текстовых файлов (.txt, .text)"
              << std::endl;
    std::cout
        << "Пожалуйста, добавьте файлы для анализа и перезапустите программу."
        << std::endl;
    std::cout << std::endl << "Нажмите Enter для выхода...";
    std::cin.get();
    return 1;
  }

  // Показываем доступные файлы
  std::cout << "Доступные текстовые файлы:" << std::endl;
  for (size_t i = 0; i < text_files.size(); i++) {
    std::cout << "  " << (i + 1) << ". " << text_files[i] << std::endl;
  }

  // Запрашиваем выбор файла
  int choice = 0;
  while (choice < 1 || choice > text_files.size()) {
    std::cout << std::endl << "Выберите файл (1-" << text_files.size() << "): ";
    std::string input;
    std::getline(std::cin, input);

    try {
      choice = std::stoi(input);
      if (choice < 1 || choice > text_files.size()) {
        std::cout << "Неверный выбор. Пожалуйста, введите число от 1 до "
                  << text_files.size() << std::endl;
      }
    } catch (...) {
      std::cout << "Неверный ввод. Пожалуйста, введите число." << std::endl;
    }
  }

  std::string selected_file = "text/" + text_files[choice - 1];
  std::cout << std::endl << "Выбран файл: " << selected_file << std::endl;

  // Инициализация детектора
  std::cout << "Инициализация детектора ..." << std::endl;

  try {
    // Загружаем модель и токенизатор
    std::string model_path = "model/bert_ner_model.onnx";
    std::string vocab_path = "model/vocab.txt";

    // Проверяем существование модели
    if (!fs::exists(model_path)) {
      std::cerr << "Ошибка: файл модели не найден: " << model_path << std::endl;
      std::cout << "Пожалуйста, убедитесь, что файл модели находится в "
                   "директории 'model'"
                << std::endl;
      std::cout << std::endl << "Нажмите Enter для выхода...";
      std::cin.get();
      return 1;
    }

    if (!fs::exists(vocab_path)) {
      std::cerr << "Ошибка: файл словаря не найден: " << vocab_path
                << std::endl;
      std::cout << "Пожалуйста, убедитесь, что файл словаря находится в "
                   "директории 'model'"
                << std::endl;
      std::cout << std::endl << "Нажмите Enter для выхода...";
      std::cin.get();
      return 1;
    }

    BertOnnxInference detector(model_path, vocab_path);
    std::cout << "Модель загружена." << std::endl;

    // Обрабатываем выбранный файл
    std::cout << "Чтение файла: " << selected_file << std::endl;

    std::string content = read_text_file(selected_file);
    if (content.empty()) {
      std::cout << "Файл пуст или не может быть прочитан." << std::endl;
      std::cout << std::endl << "Нажмите Enter для выхода...";
      std::cin.get();
      return 1;
    }

    // Отображаем информацию о размере файла
    std::cout << "Найдено символов: " << content.length() << std::endl;

    // Показываем анимированный спиннер во время обработки
    std::cout << std::endl << "Начинаю анализ..." << std::endl;

    bool processing_stopped = false;
    std::thread spinner_thread(show_processing_spinner,
                               std::ref(processing_stopped), "Анализ текста");

    // Запускаем анализ
    auto start_time = std::chrono::high_resolution_clock::now();
    auto sentences_results = detector.extract_circumstances(content);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    // Останавливаем спиннер
    processing_stopped = true;
    spinner_thread.join();

    // Считаем общее количество обстоятельств
    int total_entities = 0;
    for (const auto &sentence_result : sentences_results) {
      total_entities += sentence_result.entities.size();
    }

    std::cout << std::endl << "=== ОБРАБОТКА ЗАВЕРШЕНА ===" << std::endl;
    std::cout << "Всего предложений: " << sentences_results.size() << std::endl;
    std::cout << "Всего появлений обстоятельств: " << total_entities
              << std::endl;
    std::cout << "Время обработки: " << duration.count() << " мс" << std::endl;

    // Подсчитываем уникальные обстоятельства
    std::map<std::string, int> unique_circumstances;
    for (const auto &sentence_result : sentences_results) {
      for (const auto &entity : sentence_result.entities) {
        unique_circumstances[entity.text]++;
      }
    }
    std::cout << "Найдено уникальных обстоятельств: "
              << unique_circumstances.size() << std::endl;

    // Создаем выходную директорию
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;

#ifdef _WIN32
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
    std::string datetime_str = ss.str();

    std::string output_dir = "outputs/" + datetime_str;

    // Создаем директорию, если она не существует
    if (!fs::exists(output_dir)) {
      fs::create_directories(output_dir);
      std::cout << "Создана директория для результатов: " << output_dir
                << std::endl;
    }

    // Сохраняем результаты
    save_results(output_dir, text_files[choice - 1], sentences_results,
                 total_entities);

  } catch (const std::exception &e) {
    std::cerr << std::endl << "Ошибка: " << e.what() << std::endl;
    std::cout << std::endl << "Нажмите Enter для выхода...";
    std::cin.get();
    return 1;
  }

  std::cout << std::endl << "Нажмите Enter для выхода...";
  std::cin.get();

  return 0;
}
