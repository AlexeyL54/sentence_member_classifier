
#include "save_result.hpp"
#include "statistics.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <fstream>
#include <vector>

#ifdef WIN32
#include <windows.h>
#endif

const std::string SEARCH_FILE = "list.html";
const std::string REVIEW_FILE = "statistics.html";

size_t length(const std::string &str) {
  size_t len = 0;
  u_int bytes_to_decode_symbol;
  unsigned char first_byte_of_symbol;

  for (size_t i = 0; i < str.length();) {
    first_byte_of_symbol = static_cast<unsigned char>(str[i]);

    if (first_byte_of_symbol == 0xD0 or first_byte_of_symbol == 0xD1) {
      bytes_to_decode_symbol = 2;
    } else {
      bytes_to_decode_symbol = 1;
    }
    len++;
    i += bytes_to_decode_symbol;
  }
  return len;
}

std::string to_lower(std::string lower_str) {

  size_t len = length(lower_str);

  for (size_t i = 0; i < len;) {
    unsigned char c1 = static_cast<unsigned char>(lower_str[i]);

    // Если это начало 2-байтового символа UTF-8 (110xxxxx)
    if ((c1 & 0xE0) == 0xC0) {

      // Защита от некорректной UTF-8
      if (i + 1 >= len)
        break;

      unsigned char c2 = static_cast<unsigned char>(lower_str[i + 1]);

      // Специальные случаи (Ё, І, Є)
      if (c1 == 0xD0) {
        if (c2 == 0x81) { // Ё -> ё
          lower_str[i] = 0xD1;
          lower_str[i + 1] = 0x91;
        } else if (c2 == 0x86) { // І -> і
          lower_str[i] = 0xD1;
          lower_str[i + 1] = 0x96;
        } else if (c2 == 0x88) { // Є -> є
          lower_str[i] = 0xD1;
          lower_str[i + 1] = 0x94;
        }
        // А-П (D0 90-9F) -> а-п (D0 B0-BF)
        else if (c2 >= 0x90 && c2 <= 0x9F) {
          lower_str[i] = 0xD0;
          lower_str[i + 1] = c2 + 0x20;
        }
        // Р-Я (D0 A0-AF) -> р-я (D1 80-8F)
        else if (c2 >= 0xA0 && c2 <= 0xAF) {
          lower_str[i] = 0xD1;
          lower_str[i + 1] = c2 - 0x20;
        }
      }

      i += 2;
    } else {
      // TODO: Однобайтовый символ (ASCII) или другая длина UTF-8
      i++;
    }
  }

  return std::string(lower_str);
}

void saveHTMLWithAdvancedSearch(const std::string filename,
                                const std::vector<SearchItem> &items) {
  std::ofstream htmlFile(filename);

  if (!htmlFile.is_open()) {
    std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
    return;
  }

  htmlFile << "<!DOCTYPE html>\n";
  htmlFile << "<html lang=\"ru\">\n";
  htmlFile << "<head><meta charset=\"UTF-8\"><title>Поиск членов "
              "предложения</title>\n";
  htmlFile << "<style>\n";
  htmlFile << "body{font-family:Arial;margin:20px;background:#f5f5f5}\n";
  htmlFile << ".container{max-width:1200px;margin:0 auto}\n";
  htmlFile << ".search-box{background:#fff;padding:20px;border-radius:10px;"
              "margin-bottom:20px;box-shadow:0 2px 10px "
              "rgba(0,0,0,0.1);position:sticky;top:20px}\n";
  htmlFile << ".search-row{display:flex;gap:10px;margin-bottom:15px;flex-wrap:"
              "wrap}\n";
  htmlFile << ".search-row input{flex:2;padding:12px;font-size:16px;border:2px "
              "solid #ddd;border-radius:5px}\n";
  htmlFile
      << ".search-row select{flex:1;padding:12px;font-size:16px;border:2px "
         "solid #ddd;border-radius:5px}\n";
  htmlFile << ".search-row button{padding:12px "
              "24px;background:#4CAF50;color:#fff;border:none;border-radius:"
              "5px;cursor:pointer}\n";
  htmlFile << ".clear-btn{background:#f44336!important}\n";
  htmlFile << ".stats{margin-top:10px;color:#666;font-size:14px}\n";
  htmlFile << ".item{background:#fff;border:1px solid "
              "#ddd;border-radius:8px;margin-bottom:20px;padding:15px}\n";
  htmlFile << ".item.hidden{display:none}\n";
  htmlFile
      << ".item.highlight{background:#e8f5e9;border-left:4px solid #4CAF50}\n";
  htmlFile << ".item-header{border-bottom:2px solid "
              "#4CAF50;padding-bottom:10px;margin-bottom:15px}\n";
  htmlFile << ".text{font-size:20px;font-weight:bold;color:#2196F3}\n";
  htmlFile << ".type{display:inline-block;background:#4CAF50;color:#fff;"
              "padding:3px 10px;border-radius:5px;margin-left:10px}\n";
  htmlFile << ".sentence{background:#f9f9f9;padding:8px;margin:8px "
              "0;border-left:4px solid #4CAF50;line-height:1.6}\n";
  htmlFile << "mark{background:#FFEB3B;font-weight:bold;padding:2px "
              "4px;border-radius:3px;color:#000}\n";
  htmlFile << "h1{color:#333;text-align:center}\n";
  htmlFile << "</style></head><body>\n";
  htmlFile << "<div class=container>\n";
  htmlFile << "<h1>Поиск членов предложения</h1>\n";
  htmlFile << "<div class=search-box>\n";
  htmlFile << "<div class=search-row>\n";
  htmlFile << "<input type=text id=searchText placeholder='Введите слово для "
              "поиска'>\n";
  htmlFile << "<select id=searchType>\n";
  htmlFile << "<option value=''>Все типы</option>\n";
  htmlFile << "<option value='подлежащее'>Подлежащее</option>\n";
  htmlFile << "<option value='сказуемое'>Сказуемое</option>\n";
  htmlFile << "<option value='дополнение'>Дополнение</option>\n";
  htmlFile << "<option value='определение'>Определение</option>\n";
  htmlFile << "<option value='обстоятельство'>Обстоятельство</option>\n";
  htmlFile << "</select>\n";
  htmlFile << "<button onclick='searchWord()'>Найти</button>\n";
  htmlFile
      << "<button onclick='clearSearch()' class=clear-btn>Сброс</button>\n";
  htmlFile << "</div>\n";
  htmlFile << "<div class=stats id=stats></div>\n";
  htmlFile << "</div>\n";
  htmlFile << "<div id=results>\n";

  for (const auto &item : items) {
    htmlFile << "<div class=item data-text='" << item.text << "' data-type='"
             << item.type << "'>\n";
    htmlFile << "<div class=item-header>\n";
    htmlFile << "<span class=text>" << item.text << "</span>\n";
    htmlFile << "<span class=type>" << item.type << "</span>\n";
    htmlFile << "<div>Появлений: " << item.amount << "</div>\n";
    htmlFile << "</div>\n";
    for (const auto &sentence : item.sentences) {
      htmlFile << "<div class=sentence>" << sentence << "</div>\n";
    }
    htmlFile << "</div>\n";
  }

  htmlFile << "</div>\n";
  htmlFile << "<script>\n";
  htmlFile << "function highlightText(text, searchWord){\n";
  htmlFile << "if(!searchWord) return text;\n";
  htmlFile << "var searchLower=searchWord.toLowerCase();\n";
  htmlFile << "var textLower=text.toLowerCase();\n";
  htmlFile << "var result='';\n";
  htmlFile << "var lastIndex=0;\n";
  htmlFile << "var index=textLower.indexOf(searchLower);\n";
  htmlFile << "while(index!==-1){\n";
  htmlFile << "result+=text.substring(lastIndex,index);\n";
  htmlFile << "result+='<mark>'+text.substring(index,index+searchWord.length)+'"
              "</mark>';\n";
  htmlFile << "lastIndex=index+searchWord.length;\n";
  htmlFile << "index=textLower.indexOf(searchLower,lastIndex);\n";
  htmlFile << "}\n";
  htmlFile << "result+=text.substring(lastIndex);\n";
  htmlFile << "return result;\n";
  htmlFile << "}\n";
  htmlFile << "function searchWord(){\n";
  htmlFile
      << "var searchText=document.getElementById('searchText').value.trim();\n";
  htmlFile << "var searchType=document.getElementById('searchType').value;\n";
  htmlFile << "var items=document.querySelectorAll('.item');\n";
  htmlFile << "var found=0;\n";
  htmlFile << "for(var i=0;i<items.length;i++){\n";
  htmlFile << "var item=items[i];\n";
  htmlFile << "var text=item.getAttribute('data-text');\n";
  htmlFile << "var textLower=text.toLowerCase();\n";
  htmlFile << "var type=item.getAttribute('data-type');\n";
  htmlFile << "var searchLower=searchText.toLowerCase();\n";
  htmlFile << "var show=false;\n";
  htmlFile << "if(searchType && type!==searchType)show=false;\n";
  htmlFile << "else if(searchText && "
              "textLower.indexOf(searchLower)!==-1)show=true;\n";
  htmlFile << "else if(searchText && !show){\n";
  htmlFile << "var sentences=item.querySelectorAll('.sentence');\n";
  htmlFile << "for(var j=0;j<sentences.length;j++){\n";
  htmlFile << "var sentText=sentences[j].innerText;\n";
  htmlFile << "if(sentText.toLowerCase().indexOf(searchLower)!==-1){show=true;"
              "break;}\n";
  htmlFile << "}\n";
  htmlFile << "}else if(!searchText && !searchType)show=true;\n";
  htmlFile
      << "else if(!searchText && searchType && type===searchType)show=true;\n";
  htmlFile << "if(show){\n";
  htmlFile << "item.classList.remove('hidden');\n";
  htmlFile << "item.classList.add('highlight');\n";
  htmlFile << "found++;\n";
  htmlFile << "var textSpan=item.querySelector('.text');\n";
  htmlFile << "if(searchText && textLower.indexOf(searchLower)!==-1){\n";
  htmlFile << "textSpan.innerHTML=highlightText(text,searchText);\n";
  htmlFile << "}else{\n";
  htmlFile << "textSpan.innerHTML=text;\n";
  htmlFile << "}\n";
  htmlFile << "var sentences=item.querySelectorAll('.sentence');\n";
  htmlFile << "for(var j=0;j<sentences.length;j++){\n";
  htmlFile << "var sent=sentences[j];\n";
  htmlFile << "var originalText=sent.getAttribute('data-original');\n";
  htmlFile << "if(!originalText){\n";
  htmlFile << "sent.setAttribute('data-original',sent.innerHTML);\n";
  htmlFile << "originalText=sent.innerHTML;\n";
  htmlFile << "}\n";
  htmlFile << "if(searchText && "
              "originalText.toLowerCase().indexOf(searchLower)!==-1){\n";
  htmlFile << "sent.innerHTML=highlightText(originalText,searchText);\n";
  htmlFile << "}else{\n";
  htmlFile << "sent.innerHTML=originalText;\n";
  htmlFile << "}\n";
  htmlFile << "}\n";
  htmlFile << "}else{\n";
  htmlFile << "item.classList.add('hidden');\n";
  htmlFile << "item.classList.remove('highlight');\n";
  htmlFile << "}\n";
  htmlFile << "}\n";
  htmlFile << "var stats=document.getElementById('stats');\n";
  htmlFile << "if(searchText || searchType){\n";
  htmlFile << "if(found===0)stats.innerHTML='❌ Ничего не найдено';\n";
  htmlFile << "else stats.innerHTML='✅ Найдено '+found+' элементов по запросу "
              "\"'+searchText+'\"';\n";
  htmlFile << "}else{stats.innerHTML='';}\n";
  htmlFile << "}\n";
  htmlFile << "function clearSearch(){\n";
  htmlFile << "document.getElementById('searchText').value='';\n";
  htmlFile << "document.getElementById('searchType').value='';\n";
  htmlFile << "var items=document.querySelectorAll('.item');\n";
  htmlFile << "for(var i=0;i<items.length;i++){\n";
  htmlFile << "var item=items[i];\n";
  htmlFile << "item.classList.remove('hidden','highlight');\n";
  htmlFile << "var textSpan=item.querySelector('.text');\n";
  htmlFile << "textSpan.innerHTML=item.getAttribute('data-text');\n";
  htmlFile << "var sentences=item.querySelectorAll('.sentence');\n";
  htmlFile << "for(var j=0;j<sentences.length;j++){\n";
  htmlFile << "var sent=sentences[j];\n";
  htmlFile << "if(sent.getAttribute('data-original')){\n";
  htmlFile << "sent.innerHTML=sent.getAttribute('data-original');\n";
  htmlFile << "}\n";
  htmlFile << "}\n";
  htmlFile << "}\n";
  htmlFile << "document.getElementById('stats').innerHTML='';\n";
  htmlFile << "}\n";
  htmlFile << "document.getElementById('searchText').addEventListener('"
              "keypress',function(e){if(e.key==='Enter')searchWord();});\n";
  htmlFile << "document.getElementById('searchType').addEventListener('change',"
              "function(){searchWord();});\n";
  htmlFile << "</script>\n";
  htmlFile << "</body></html>\n";

  htmlFile.close();
}

void generateHTMLCharts(const std::string filename, const GlobalStats &stats) {
  std::ofstream htmlFile(filename);

  if (!htmlFile.is_open()) {
    std::cerr << "Ошибка: не удалось открыть файл " << filename << std::endl;
    return;
  }

  htmlFile << "<!DOCTYPE html>\n";
  htmlFile << "<html lang=\"ru\">\n";
  htmlFile << "<head>\n";
  htmlFile << "    <meta charset=\"UTF-8\">\n";
  htmlFile << "    <meta name=\"viewport\" content=\"width=device-width, "
              "initial-scale=1.0\">\n";
  htmlFile << "    <title>Статистика синтаксического анализа текста</title>\n";
  htmlFile << "    <script "
              "src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/"
              "chart.umd.min.js\"></script>\n";
  htmlFile << "    <style>\n";
  htmlFile << "        body {\n";
  htmlFile << "            font-family: 'Segoe UI', Arial, sans-serif;\n";
  htmlFile << "            margin: 20px;\n";
  htmlFile << "            background-color: #f5f5f5;\n";
  htmlFile << "        }\n";
  htmlFile << "        .container {\n";
  htmlFile << "            max-width: 1200px;\n";
  htmlFile << "            margin: 0 auto;\n";
  htmlFile << "        }\n";
  htmlFile << "        h1 {\n";
  htmlFile << "            color: #333;\n";
  htmlFile << "            text-align: center;\n";
  htmlFile << "            font-weight: normal;\n";
  htmlFile << "        }\n";
  htmlFile << "        .stats-summary {\n";
  htmlFile << "            display: grid;\n";
  htmlFile
      << "            grid-template-columns: repeat(auto-fit, minmax(200px, "
         "1fr));\n";
  htmlFile << "            gap: 20px;\n";
  htmlFile << "            margin-bottom: 30px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .stat-card {\n";
  htmlFile << "            background: white;\n";
  htmlFile << "            border: 1px solid #ddd;\n";
  htmlFile << "            border-radius: 4px;\n";
  htmlFile << "            padding: 20px;\n";
  htmlFile << "            text-align: center;\n";
  htmlFile << "        }\n";
  htmlFile << "        .stat-value {\n";
  htmlFile << "            font-size: 32px;\n";
  htmlFile << "            font-weight: bold;\n";
  htmlFile << "            color: #333;\n";
  htmlFile << "        }\n";
  htmlFile << "        .stat-label {\n";
  htmlFile << "            color: #666;\n";
  htmlFile << "            margin-top: 5px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .charts-grid {\n";
  htmlFile << "            display: grid;\n";
  htmlFile
      << "            grid-template-columns: repeat(auto-fit, minmax(400px, "
         "1fr));\n";
  htmlFile << "            gap: 20px;\n";
  htmlFile << "            margin-bottom: 30px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .chart-card {\n";
  htmlFile << "            background: white;\n";
  htmlFile << "            border: 1px solid #ddd;\n";
  htmlFile << "            border-radius: 4px;\n";
  htmlFile << "            padding: 20px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .chart-card h3 {\n";
  htmlFile << "            text-align: center;\n";
  htmlFile << "            color: #333;\n";
  htmlFile << "            margin-bottom: 20px;\n";
  htmlFile << "            font-weight: normal;\n";
  htmlFile << "        }\n";
  htmlFile << "        canvas {\n";
  htmlFile << "            max-height: 300px;\n";
  htmlFile << "            margin: 0 auto;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-items {\n";
  htmlFile << "            background: white;\n";
  htmlFile << "            border: 1px solid #ddd;\n";
  htmlFile << "            border-radius: 4px;\n";
  htmlFile << "            padding: 20px;\n";
  htmlFile << "            margin-top: 20px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-items h3 {\n";
  htmlFile << "            text-align: center;\n";
  htmlFile << "            color: #333;\n";
  htmlFile << "            margin-bottom: 20px;\n";
  htmlFile << "            font-weight: normal;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-grid {\n";
  htmlFile << "            display: grid;\n";
  htmlFile
      << "            grid-template-columns: repeat(auto-fit, minmax(200px, "
         "1fr));\n";
  htmlFile << "            gap: 15px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-item {\n";
  htmlFile << "            background-color: #f9f9f9;\n";
  htmlFile << "            border-radius: 4px;\n";
  htmlFile << "            padding: 12px;\n";
  htmlFile << "            text-align: center;\n";
  htmlFile << "            border-left: 3px solid #999;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-item .part {\n";
  htmlFile << "            font-size: 14px;\n";
  htmlFile << "            color: #666;\n";
  htmlFile << "            margin-bottom: 6px;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-item .word {\n";
  htmlFile << "            font-weight: bold;\n";
  htmlFile << "            font-size: 18px;\n";
  htmlFile << "            color: #333;\n";
  htmlFile << "            margin: 5px 0;\n";
  htmlFile << "        }\n";
  htmlFile << "        .top-item .count {\n";
  htmlFile << "            font-size: 20px;\n";
  htmlFile << "            font-weight: bold;\n";
  htmlFile << "            color: #333;\n";
  htmlFile << "        }\n";
  htmlFile << "        .footer {\n";
  htmlFile << "            text-align: center;\n";
  htmlFile << "            margin-top: 30px;\n";
  htmlFile << "            color: #999;\n";
  htmlFile << "            font-size: 12px;\n";
  htmlFile << "        }\n";
  htmlFile << "    </style>\n";
  htmlFile << "</head>\n";
  htmlFile << "<body>\n";
  htmlFile << "<div class=\"container\">\n";
  htmlFile << "    <h1>Статистика синтаксического анализа текста</h1>\n";
  htmlFile << "    \n";
  htmlFile << "    <div class=\"stats-summary\">\n";
  htmlFile << "        <div class=\"stat-card\">\n";
  htmlFile << "            <div class=\"stat-value\">" << stats.sentences_total
           << "</div>\n";
  htmlFile << "            <div class=\"stat-label\">Предложений</div>\n";
  htmlFile << "        </div>\n";
  htmlFile << "        <div class=\"stat-card\">\n";
  htmlFile << "            <div class=\"stat-value\">" << stats.words_total
           << "</div>\n";
  htmlFile << "            <div class=\"stat-label\">Слов</div>\n";
  htmlFile << "        </div>\n";
  htmlFile << "        <div class=\"stat-card\">\n";
  htmlFile << "            <div class=\"stat-value\">" << stats.members_total
           << "</div>\n";
  htmlFile
      << "            <div class=\"stat-label\">Членов предложения</div>\n";
  htmlFile << "        </div>\n";
  htmlFile << "    </div>\n";
  htmlFile << "    \n";
  htmlFile << "    <div class=\"charts-grid\">\n";
  htmlFile << "        <div class=\"chart-card\">\n";
  htmlFile << "            <h3>Распределение членов предложения</h3>\n";
  htmlFile << "            <canvas id=\"membersChart\"></canvas>\n";
  htmlFile << "        </div>\n";
  htmlFile << "    </div>\n";
  htmlFile << "    \n";
  htmlFile << "    <div class=\"top-items\">\n";
  htmlFile << "        <h3>Самые популярные слова</h3>\n";
  htmlFile << "        <div class=\"top-grid\">\n";
  htmlFile << "            <div class=\"top-item\">\n";
  htmlFile << "                <div class=\"part\">Подлежащее</div>\n";
  htmlFile << "                <div class=\"word\">«" << stats.top_subject.first
           << "»</div>\n";
  htmlFile << "                <div class=\"count\">"
           << stats.top_subject.second << " раз(а)</div>\n";
  htmlFile << "            </div>\n";
  htmlFile << "            <div class=\"top-item\">\n";
  htmlFile << "                <div class=\"part\">Сказуемое</div>\n";
  htmlFile << "                <div class=\"word\">«"
           << stats.top_predicate.first << "»</div>\n";
  htmlFile << "                <div class=\"count\">"
           << stats.top_predicate.second << " раз(а)</div>\n";
  htmlFile << "            </div>\n";
  htmlFile << "            <div class=\"top-item\">\n";
  htmlFile << "                <div class=\"part\">Определение</div>\n";
  htmlFile << "                <div class=\"word\">«"
           << stats.top_definition.first << "»</div>\n";
  htmlFile << "                <div class=\"count\">"
           << stats.top_definition.second << " раз(а)</div>\n";
  htmlFile << "            </div>\n";
  htmlFile << "            <div class=\"top-item\">\n";
  htmlFile << "                <div class=\"part\">Дополнение</div>\n";
  htmlFile << "                <div class=\"word\">«"
           << stats.top_addition.first << "»</div>\n";
  htmlFile << "                <div class=\"count\">"
           << stats.top_addition.second << " раз(а)</div>\n";
  htmlFile << "            </div>\n";
  htmlFile << "            <div class=\"top-item\">\n";
  htmlFile << "                <div class=\"part\">Обстоятельство</div>\n";
  htmlFile << "                <div class=\"word\">«"
           << stats.top_adverbial.first << "»</div>\n";
  htmlFile << "                <div class=\"count\">"
           << stats.top_adverbial.second << " раз(а)</div>\n";
  htmlFile << "            </div>\n";
  htmlFile << "        </div>\n";
  htmlFile << "    </div>\n";
  htmlFile << "    \n";
  htmlFile << "    <div class=\"footer\">\n";
  htmlFile << "        Giga Голиков AI\n";
  htmlFile << "    </div>\n";
  htmlFile << "</div>\n";
  htmlFile << "\n";
  htmlFile << "<script>\n";
  htmlFile << "    const membersCtx = "
              "document.getElementById('membersChart').getContext('2d');\n";
  htmlFile << "    new Chart(membersCtx, {\n";
  htmlFile << "        type: 'pie',\n";
  htmlFile << "        data: {\n";
  htmlFile << "            labels: [\n";
  htmlFile << "                'Подлежащие (" << stats.subjects_total
           << ")',\n";
  htmlFile << "                'Сказуемые (" << stats.predicates_total
           << ")',\n";
  htmlFile << "                'Определения (" << stats.definitions_total
           << ")',\n";
  htmlFile << "                'Дополнения (" << stats.additions_total
           << ")',\n";
  htmlFile << "                'Обстоятельства (" << stats.adverbials_total
           << ")'\n";
  htmlFile << "            ],\n";
  htmlFile << "            datasets: [{\n";
  htmlFile << "                data: [" << stats.subjects_total << ", "
           << stats.predicates_total << ", " << stats.definitions_total << ", "
           << stats.additions_total << ", " << stats.adverbials_total << "],\n";
  htmlFile
      << "                backgroundColor: ['#4ECDC4', '#45B7D1', '#96CEB4', "
         "'#FFEAA7', '#FF6B6B'],\n";
  htmlFile << "                borderWidth: 1,\n";
  htmlFile << "                borderColor: '#fff'\n";
  htmlFile << "            }]\n";
  htmlFile << "        },\n";
  htmlFile << "        options: {\n";
  htmlFile << "            responsive: true,\n";
  htmlFile << "            maintainAspectRatio: true,\n";
  htmlFile << "            plugins: {\n";
  htmlFile << "                legend: { position: 'bottom' },\n";
  htmlFile << "                tooltip: {\n";
  htmlFile << "                    callbacks: {\n";
  htmlFile << "                        label: function(context) {\n";
  htmlFile
      << "                            const label = context.label || '';\n";
  htmlFile
      << "                            const value = context.parsed || 0;\n";
  htmlFile << "                            const total = "
              "context.dataset.data.reduce((a, b) => a + b, 0);\n";
  htmlFile << "                            const percent = ((value / total) * "
              "100).toFixed(1);\n";
  htmlFile
      << "                            return label + ': ' + value + ' (' + "
         "percent + '%)';\n";
  htmlFile << "                        }\n";
  htmlFile << "                    }\n";
  htmlFile << "                }\n";
  htmlFile << "            }\n";
  htmlFile << "        }\n";
  htmlFile << "    });\n";
  htmlFile << "</script>\n";
  htmlFile << "</body>\n";
  htmlFile << "</html>\n";

  htmlFile.close();
}

void saveAnalysis(const std::string path, std::vector<SearchItem> &items,
                  GlobalStats &stats) {

  saveHTMLWithAdvancedSearch(path + "/" + SEARCH_FILE, items);
  generateHTMLCharts(path + "/" + REVIEW_FILE, stats);
}
