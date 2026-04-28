/**
 * @file save_result.cpp
 * @brief Реализация функций для сохранения результатов анализа в HTML
 *
 * Этот файл содержит функции для создания HTML-страниц с результатами
 * синтаксического анализа текста, включая интерактивный поиск и графики.
 */

#include "save_result.hpp"
#include "statistics.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef WIN32
#include <windows.h>
#endif

/**
 * @brief Имя файла с результатами поиска
 */
const std::string SEARCH_FILE = "list.html";

/**
 * @brief Имя файла со статистикой
 */
const std::string REVIEW_FILE = "statistics.html";

/**
 * @brief Имя директории с шаблонами
 */
const std::string TEMPLATE_DIR = "templates/";

/**
 * @brief Загружает содержимое файла-шаблона
 *
 * @param filename Имя файла шаблона
 * @return std::string Содержимое файла
 */
std::string loadTemplateFile(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Ошибка: не удалось открыть файл шаблона: " << filename
              << std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

/**
 * @brief Копирует файл ресурсов (CSS/JS) в выходную директорию
 *
 * @param source_path Путь к исходному файлу
 * @param dest_path Путь к целевому файлу
 */
void copyResourceFile(const std::string &source_path,
                      const std::string &dest_path) {
  std::ifstream src(source_path, std::ios::binary);
  std::ofstream dst(dest_path, std::ios::binary);

  if (!src.is_open() || !dst.is_open()) {
    std::cerr << "Ошибка: не удалось скопировать файл ресурсов: " << source_path
              << std::endl;
    return;
  }

  dst << src.rdbuf();
}

/**
 * @brief Копирует все необходимые ресурсы (CSS, JS) в выходную директорию
 *
 * @param output_dir Директория для сохранения ресурсов
 */
void copyResources(const std::string &output_dir) {
  copyResourceFile(TEMPLATE_DIR + "search_styles.css",
                   output_dir + "/search_styles.css");
  copyResourceFile(TEMPLATE_DIR + "stats_styles.css",
                   output_dir + "/stats_styles.css");
  copyResourceFile(TEMPLATE_DIR + "search_script.js",
                   output_dir + "/search_script.js");
}

/**
 * @brief Генерирует HTML-контент для элементов поиска
 *
 * @param items Вектор элементов поиска
 * @return std::string HTML-разметка элементов
 */
std::string generateSearchItemsHTML(const std::vector<SearchItem> &items) {
  std::stringstream html;

  for (const auto &item : items) {
    html << "<div class='item' data-text='" << item.text << "' data-type='"
         << item.type << "'>\n";
    html << "    <div class='item-header'>\n";
    html << "        <span class='text'>" << item.text << "</span>\n";
    html << "        <span class='type'>" << item.type << "</span>\n";
    html << "        <div>Появлений: " << item.amount << "</div>\n";
    html << "    </div>\n";

    for (const auto &sentence : item.sentences) {
      html << "    <div class='sentence'>" << sentence.second << "</div>\n";
    }
    html << "</div>\n";
  }

  return html.str();
}

/**
 * @brief Генерирует HTML-контент для статистической сводки
 *
 * @param stats Глобальная статистика
 * @return std::string HTML-разметка статистической сводки
 */
std::string generateStatsSummaryHTML(const GlobalStats &stats) {
  std::stringstream html;

  html << "<div class='stat-card'>\n";
  html << "    <div class='stat-value'>" << stats.sentences_total << "</div>\n";
  html << "    <div class='stat-label'>Предложений</div>\n";
  html << "</div>\n";

  html << "<div class='stat-card'>\n";
  html << "    <div class='stat-value'>" << stats.words_total << "</div>\n";
  html << "    <div class='stat-label'>Слов</div>\n";
  html << "</div>\n";

  html << "<div class='stat-card'>\n";
  html << "    <div class='stat-value'>" << stats.members_total << "</div>\n";
  html << "    <div class='stat-label'>Членов предложения</div>\n";
  html << "</div>\n";

  return html.str();
}

/**
 * @brief Генерирует HTML-контент для популярных слов
 *
 * @param stats Глобальная статистика
 * @return std::string HTML-разметка популярных слов
 */
std::string generateTopItemsHTML(const GlobalStats &stats) {
  std::stringstream html;

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Подлежащее</div>\n";
  html << "    <div class='word'>«" << stats.top_subject.first << "»</div>\n";
  html << "    <div class='count'>" << stats.top_subject.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Сказуемое</div>\n";
  html << "    <div class='word'>«" << stats.top_predicate.first << "»</div>\n";
  html << "    <div class='count'>" << stats.top_predicate.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Определение</div>\n";
  html << "    <div class='word'>«" << stats.top_definition.first
       << "»</div>\n";
  html << "    <div class='count'>" << stats.top_definition.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Дополнение</div>\n";
  html << "    <div class='word'>«" << stats.top_addition.first << "»</div>\n";
  html << "    <div class='count'>" << stats.top_addition.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Обстоятельство</div>\n";
  html << "    <div class='word'>«" << stats.top_adverbial.first << "»</div>\n";
  html << "    <div class='count'>" << stats.top_adverbial.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  return html.str();
}

/**
 * @brief Генерирует JavaScript-код для графика статистики
 *
 * @param stats Глобальная статистика
 * @return std::string JavaScript-код для инициализации графика
 */
std::string generateChartScript(const GlobalStats &stats) {
  std::stringstream script;

  script << "    const membersCtx = "
            "document.getElementById('membersChart').getContext('2d');\n";
  script << "    new Chart(membersCtx, {\n";
  script << "        type: 'pie',\n";
  script << "        data: {\n";
  script << "            labels: [\n";
  script << "                'Подлежащие (" << stats.subjects_total << ")',\n";
  script << "                'Сказуемые (" << stats.predicates_total << ")',\n";
  script << "                'Определения (" << stats.definitions_total
         << ")',\n";
  script << "                'Дополнения (" << stats.additions_total << ")',\n";
  script << "                'Обстоятельства (" << stats.adverbials_total
         << ")'\n";
  script << "            ],\n";
  script << "            datasets: [{\n";
  script << "                data: [" << stats.subjects_total << ", "
         << stats.predicates_total << ", " << stats.definitions_total << ", "
         << stats.additions_total << ", " << stats.adverbials_total << "],\n";
  script << "                backgroundColor: ['#4ECDC4', '#45B7D1', "
            "'#96CEB4', '#FFEAA7', '#FF6B6B'],\n";
  script << "                borderWidth: 1,\n";
  script << "                borderColor: '#fff'\n";
  script << "            }]\n";
  script << "        },\n";
  script << "        options: {\n";
  script << "            responsive: true,\n";
  script << "            maintainAspectRatio: true,\n";
  script << "            plugins: {\n";
  script << "                legend: { position: 'bottom' },\n";
  script << "                tooltip: {\n";
  script << "                    callbacks: {\n";
  script << "                        label: function(context) {\n";
  script << "                            const label = context.label || '';\n";
  script << "                            const value = context.parsed || 0;\n";
  script << "                            const total = "
            "context.dataset.data.reduce((a, b) => a + b, 0);\n";
  script << "                            const percent = ((value / total) * "
            "100).toFixed(1);\n";
  script << "                            return label + ': ' + value + ' (' + "
            "percent + '%)';\n";
  script << "                        }\n";
  script << "                    }\n";
  script << "                }\n";
  script << "            }\n";
  script << "        }\n";
  script << "    });\n";

  return script.str();
}

/**
 * @brief Сохраняет HTML-страницу с расширенным поиском
 *
 * @param filename Имя выходного файла
 * @param items Вектор элементов поиска
 */
void saveHTMLWithAdvancedSearch(const std::string filename,
                                const std::vector<SearchItem> &items) {
  std::string htmlTemplate =
      loadTemplateFile(TEMPLATE_DIR + "search_template.html");
  if (htmlTemplate.empty()) {
    std::cerr << "Ошибка: не удалось загрузить шаблон поиска" << std::endl;
    return;
  }

  std::string itemsHTML = generateSearchItemsHTML(items);

  // Вставляем элементы в шаблон
  size_t resultsPos = htmlTemplate.find("<div id=\"results\">");
  if (resultsPos != std::string::npos) {
    resultsPos += strlen("<div id=\"results\">");
    htmlTemplate.insert(resultsPos, itemsHTML + "\n        ");
  }

  std::ofstream htmlFile(filename);
  if (!htmlFile.is_open()) {
    std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
    return;
  }

  htmlFile << htmlTemplate;
  htmlFile.close();
}

/**
 * @brief Генерирует HTML-страницу со статистикой и графиками
 *
 * @param filename Имя выходного файла
 * @param stats Глобальная статистика
 */
void generateHTMLCharts(const std::string filename, const GlobalStats &stats) {
  std::string htmlTemplate =
      loadTemplateFile(TEMPLATE_DIR + "stats_template.html");
  if (htmlTemplate.empty()) {
    std::cerr << "Ошибка: не удалось загрузить шаблон статистики" << std::endl;
    return;
  }

  std::string summaryHTML = generateStatsSummaryHTML(stats);
  std::string topItemsHTML = generateTopItemsHTML(stats);
  std::string chartScript = generateChartScript(stats);

  // Вставляем статистическую сводку
  size_t summaryPos =
      htmlTemplate.find("<div class=\"stats-summary\" id=\"statsSummary\">");
  if (summaryPos != std::string::npos) {
    summaryPos += strlen("<div class=\"stats-summary\" id=\"statsSummary\">");
    htmlTemplate.insert(summaryPos, summaryHTML + "\n        ");
  }

  // Вставляем популярные слова
  size_t topItemsPos =
      htmlTemplate.find("<div class=\"top-items\" id=\"topItems\">");
  if (topItemsPos != std::string::npos) {
    topItemsPos += strlen("<div class=\"top-items\" id=\"topItems\">");
    size_t closingDiv = htmlTemplate.find("</div>", topItemsPos);
    if (closingDiv != std::string::npos) {
      htmlTemplate.insert(closingDiv, topItemsHTML + "\n        ");
    }
  }

  // Вставляем скрипт графика
  size_t scriptPos = htmlTemplate.find("</script>");
  if (scriptPos != std::string::npos) {
    htmlTemplate.insert(scriptPos, chartScript);
  }

  std::ofstream htmlFile(filename);
  if (!htmlFile.is_open()) {
    std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
    return;
  }

  htmlFile << htmlTemplate;
  htmlFile.close();
}

/**
 * @brief Сохраняет результаты анализа в HTML файлы
 *
 * @param path Путь к директории для сохранения
 * @param items Вектор элементов поиска
 * @param stats Глобальная статистика
 */
void saveAnalysis(const std::string path, std::vector<SearchItem> &items,
                  GlobalStats &stats) {
  // Копируем ресурсы (CSS, JS)
  copyResources(path);

  // Генерируем HTML страницы
  saveHTMLWithAdvancedSearch(path + "/" + SEARCH_FILE, items);
  generateHTMLCharts(path + "/" + REVIEW_FILE, stats);
}
