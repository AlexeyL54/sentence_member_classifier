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
const std::string STATS_FILE = "statistics.html";

/**
 * @brief Имя директории с шаблонами
 */
const std::string TEMPLATE_DIR = "../resources/templates/";

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
 * @brief Загружает CSS файл в виде строки
 *
 * @param filename Имя CSS файла
 * @return std::string Содержимое CSS
 */
std::string loadCSS(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Ошибка: не удалось открыть CSS файл: " << filename
              << std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

/**
 * @brief Загружает JS файл в виде строки
 *
 * @param filename Имя JS файла
 * @return std::string Содержимое JS
 */
std::string loadJS(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Ошибка: не удалось открыть JS файл: " << filename
              << std::endl;
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

/**
 * @brief Генерирует HTML-контент для элементов поиска
 *
 * @param items Вектор элементов поиска
 * @return std::string HTML-разметка элементов
 */
/**
 * @brief Генерирует HTML-контент для элементов поиска
 *
 * @param items Вектор элементов поиска
 * @return std::string HTML-разметка элементов
 */
std::string generateSearchItemsHTML(const std::vector<SearchItem> &items) {
  std::stringstream html;
  int orderIndex = 0;

  for (const SearchItem &item : items) {
    // Экранируем кавычки для безопасного вставки в атрибуты
    std::string escapedText = item.text.toUtf8().toStdString();
    std::string escapedType = item.type.toUtf8().toStdString();

    size_t pos = 0;
    while ((pos = escapedText.find('"', pos)) != std::string::npos) {
      escapedText.replace(pos, 1, "&quot;");
      pos += 6;
    }

    while ((pos = escapedType.find('"', pos)) != std::string::npos) {
      escapedType.replace(pos, 1, "&quot;");
      pos += 6;
    }

    html << "<div class='item' data-text='" << escapedText << "' data-type='"
         << escapedType << "' data-original-order='" << orderIndex++ << "'>\n";
    html << "    <div class='item-header'>\n";
    html << "        <span class='text'>" << item.text.toUtf8().toStdString()
         << "</span>\n";
    html << "        <span class='type'>" << item.text.toUtf8().toStdString()
         << "</span>\n";
    html << "        <div>Появлений: " << item.amount << "</div>\n";
    html << "    </div>\n";

    for (std::pair<int, QString> const &sentence : item.sentences) {
      std::string sentText = sentence.second.toUtf8().toStdString();
      // Экранируем для безопасного вставки
      std::string escapedSent = sentText;
      pos = 0;
      while ((pos = escapedSent.find('"', pos)) != std::string::npos) {
        escapedSent.replace(pos, 1, "&quot;");
        pos += 6;
      }
      html << "    <div class='sentence' data-original=\"" << escapedSent
           << "\">" << sentText << "</div>\n";
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
/**
 * @brief Генерирует HTML-контент для популярных слов
 *
 * @param stats Глобальная статистика
 * @return std::string HTML-разметка популярных слов
 */
std::string generateTopItemsHTML(const GlobalStats &stats) {
  std::stringstream html;

  html << "<h3>Самые популярные члены предложения</h3>\n";
  html << "<div class='top-grid'>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Подлежащее</div>\n";
  html << "    <div class='word'>«"
       << stats.top_subject.first.toUtf8().toStdString() << "»</div>\n";
  html << "    <div class='count'>" << stats.top_subject.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Сказуемое</div>\n";
  html << "    <div class='word'>«"
       << stats.top_predicate.first.toUtf8().toStdString() << "»</div>\n";
  html << "    <div class='count'>" << stats.top_predicate.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Определение</div>\n";
  html << "    <div class='word'>«"
       << stats.top_definition.first.toUtf8().toStdString() << "»</div>\n";
  html << "    <div class='count'>" << stats.top_definition.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Дополнение</div>\n";
  html << "    <div class='word'>«"
       << stats.top_addition.first.toUtf8().toStdString() << "»</div>\n";
  html << "    <div class='count'>" << stats.top_addition.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Обстоятельство</div>\n";
  html << "    <div class='word'>«"
       << stats.top_adverbial.first.toUtf8().toStdString() << "»</div>\n";
  html << "    <div class='count'>" << stats.top_adverbial.second
       << " раз(а)</div>\n";
  html << "</div>\n";

  // Добавляем категорию "Другое"
  html << "<div class='top-item'>\n";
  html << "    <div class='part'>Другое</div>\n";
  html << "    <div class='word'>«"
       << stats.top_other.first.toUtf8().toStdString() << "»</div>\n";
  html << "    <div class='count'>" << stats.top_other.second
       << " раз(а)</div>\n";
  html << "</div>\n";

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

  script << "    document.addEventListener('DOMContentLoaded', function() {\n";
  script << "        var membersCtx = "
            "document.getElementById('membersChart').getContext('2d');\n";
  script << "        new Chart(membersCtx, {\n";
  script << "            type: 'pie',\n";
  script << "            data: {\n";
  script << "                labels: [\n";
  script << "                    'Подлежащие (" << stats.subjects_total
         << ")',\n";
  script << "                    'Сказуемые (" << stats.predicates_total
         << ")',\n";
  script << "                    'Определения (" << stats.definitions_total
         << ")',\n";
  script << "                    'Дополнения (" << stats.additions_total
         << ")',\n";
  script << "                    'Обстоятельства (" << stats.adverbials_total
         << ")',\n";
  script << "                    'Другое (" << stats.others_total << ")'\n";
  script << "                ],\n";
  script << "                datasets: [{\n";
  script << "                    data: [" << stats.subjects_total << ", "
         << stats.predicates_total << ", " << stats.definitions_total << ", "
         << stats.additions_total << ", " << stats.adverbials_total << ", "
         << stats.others_total << "],\n";
  script << "                    backgroundColor: ['#4ECDC4', '#45B7D1', "
            "'#96CEB4', '#FFEAA7', '#FF6B6B', '#9B59B6'],\n";
  script << "                    borderWidth: 1,\n";
  script << "                    borderColor: '#fff'\n";
  script << "                }]\n";
  script << "            },\n";
  script << "            options: {\n";
  script << "                responsive: true,\n";
  script << "                maintainAspectRatio: true,\n";
  script << "                plugins: {\n";
  script << "                    legend: { position: 'bottom' },\n";
  script << "                    tooltip: {\n";
  script << "                        callbacks: {\n";
  script << "                            label: function(context) {\n";
  script
      << "                                var label = context.label || '';\n";
  script
      << "                                var value = context.parsed || 0;\n";
  script
      << "                                var total = "
         "context.dataset.data.reduce(function(a, b) { return a + b; }, 0);\n";
  script << "                                var percent = ((value / total) * "
            "100).toFixed(1);\n";
  script
      << "                                return label + ': ' + value + ' (' + "
         "percent + '%)';\n";
  script << "                            }\n";
  script << "                        }\n";
  script << "                    }\n";
  script << "                }\n";
  script << "            }\n";
  script << "        });\n";
  script << "    });\n";

  return script.str();
}

/**
 * @brief Сохраняет HTML-страницу с расширенным поиском
 *
 * @param filename Имя выходного файла
 * @param items Вектор элементов поиска
 */
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

  // Загружаем CSS и JS
  std::string cssContent = loadCSS(TEMPLATE_DIR + "search_styles.css");
  std::string jsContent = loadJS(TEMPLATE_DIR + "search_script.js");

  // Вставляем CSS в head
  std::string styleTag = "<style>\n" + cssContent + "\n</style>";
  size_t headPos = htmlTemplate.find("</head>");
  if (headPos != std::string::npos) {
    htmlTemplate.insert(headPos, styleTag + "\n    ");
  }

  // Вставляем JS перед закрывающим body
  std::string scriptTag = "<script>\n" + jsContent + "\n</script>";
  size_t bodyPos = htmlTemplate.find("</body>");
  if (bodyPos != std::string::npos) {
    htmlTemplate.insert(bodyPos, scriptTag + "\n    ");
  }

  std::string itemsHTML = generateSearchItemsHTML(items);

  // Вставляем элементы в шаблон
  size_t resultsPos = htmlTemplate.find("<div id=\"results\">");
  if (resultsPos != std::string::npos) {
    resultsPos += strlen("<div id=\"results\">");
    htmlTemplate.insert(resultsPos,
                        "\n            " + itemsHTML + "\n        ");
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

  // Загружаем CSS
  std::string cssContent = loadCSS(TEMPLATE_DIR + "stats_styles.css");

  // Вставляем CSS в head
  std::string styleTag = "<style>\n" + cssContent + "\n</style>";
  size_t headPos = htmlTemplate.find("</head>");
  if (headPos != std::string::npos) {
    htmlTemplate.insert(headPos, styleTag + "\n    ");
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

  // Вставляем скрипт графика перед закрывающим body
  std::string scriptTag = "<script>\n" + chartScript + "\n</script>";
  size_t bodyPos = htmlTemplate.find("</body>");
  if (bodyPos != std::string::npos) {
    htmlTemplate.insert(bodyPos, scriptTag + "\n    ");
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
  // Генерируем HTML страницы (все ресурсы встроены)
  saveHTMLWithAdvancedSearch(path + "/" + SEARCH_FILE, items);
  generateHTMLCharts(path + "/" + STATS_FILE, stats);
}
