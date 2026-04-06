#include <iostream>
#include <string>
#include <windows.h>
//#include "../../src/back/statistics.hpp"
//#include "../../src/back/save_result.hpp"


#include <map>
#include <vector>
#include <sstream>


// Вектор из 10 заполненных элементов



#include <vector>
#include <sstream>
#include <fstream>

struct SearchItem {
  std::string text; // член предложения
  std::string type; // вид члена предложения (подлежащее, сказуемое, ...)
  std::vector<std::string>
      sentences;  // предложения, в которых встречается этот член предложения
  int amount = 0; // количество появлений в тексте
};

struct GlobalStats {
  int sentences_total = 0; // количество предложений в тексте
  int words_total =
      0; // количество слов в тексте (подсчёт по SentenceResult::text)
  int members_total = 0;     // количество членов предложения в тексте
  int subjects_total = 0;    // количество подлежащих в тексте
  int predicates_total = 0;  // количество сказуемых в тексте
  int definitions_total = 0; // количество определений в тексте
  int additions_total = 0;   // количество дополнений в тексте
  int adverbials_total = 0;  // количество обстоятельств в тексте
  std::pair<std::string, int> top_subject;    // самое популярное подлежащее
  std::pair<std::string, int> top_predicate;  // самое популярное сказуемое
  std::pair<std::string, int> top_definition; // самое популярное определение
  std::pair<std::string, int> top_addition;   // самое популярное дополнение
  std::pair<std::string, int> top_adverbial;  // самое популярное обстоятельство
};

void saveHTMLWithAdvancedSearch(const std::vector<SearchItem> &items,
                                const std::string &filename) {
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
/*
std::string generateHTMLCharts(const GlobalStats& stats) {
    std::stringstream html;
    
    html << R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Статистика синтаксического анализа текста</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 1400px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            color: white;
            margin-bottom: 30px;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .stats-summary {
            background: white;
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 30px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
        }
        .stat-card {
            background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
            border-radius: 10px;
            padding: 15px;
            text-align: center;
            transition: transform 0.3s;
        }
        .stat-card:hover {
            transform: translateY(-5px);
        }
        .stat-value {
            font-size: 28px;
            font-weight: bold;
            color: #333;
        }
        .stat-label {
            font-size: 14px;
            color: #666;
            margin-top: 5px;
        }
        .charts-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 25px;
            margin-bottom: 30px;
        }
        .chart-card {
            background: white;
            border-radius: 15px;
            padding: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.2);
        }
        .chart-card h3 {
            text-align: center;
            color: #333;
            margin-bottom: 20px;
        }
        canvas {
            max-height: 300px;
            margin: 0 auto;
        }
        .top-items {
            background: white;
            border-radius: 15px;
            padding: 20px;
            margin-top: 20px;
        }
        .top-items h3 {
            text-align: center;
            color: #333;
            margin-bottom: 20px;
        }
        .top-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 15px;
        }
        .top-item {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 10px;
            text-align: center;
            border-left: 4px solid #667eea;
        }
        .top-word {
            font-weight: bold;
            font-size: 16px;
            color: #333;
        }
        .top-count {
            color: #667eea;
            font-weight: bold;
            font-size: 20px;
        }
        @media (max-width: 768px) {
            .charts-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📊 Статистика синтаксического анализа текста</h1>
        
        <div class="stats-summary">
            <div class="stat-card">
                <div class="stat-value">)" << stats.sentences_total << R"(</div>
                <div class="stat-label">📝 Предложений</div>
            </div>
            <div class="stat-card">
                <div class="stat-value">)" << stats.words_total << R"(</div>
                <div class="stat-label">📖 Слов</div>
            </div>
            <div class="stat-card">
                <div class="stat-value">)" << stats.members_total << R"(</div>
                <div class="stat-label">🔤 Членов предложения</div>
            </div>
        </div>
        
        <div class="charts-grid">
            <div class="chart-card">
                <h3>📊 Распределение членов предложения</h3>
                <canvas id="membersChart"></canvas>
            </div>
            <div class="chart-card">
                <h3>⭐ Самые частотные элементы</h3>
                <canvas id="topElementsChart"></canvas>
            </div>
        </div>
        
        <div class="top-items">
            <h3>🏆 Самые популярные слова</h3>
            <div class="top-grid">
                <div class="top-item">
                    <div class="top-word">📖 Подлежащее</div>
                    <div class="top-word">«)" << stats.top_subject.first << R"(»</div>
                    <div class="top-count">)" << stats.top_subject.second << R"( раз(а)</div>
                </div>
                <div class="top-item">
                    <div class="top-word">⚡ Сказуемое</div>
                    <div class="top-word">«)" << stats.top_predicate.first << R"(»</div>
                    <div class="top-count">)" << stats.top_predicate.second << R"( раз(а)</div>
                </div>
                <div class="top-item">
                    <div class="top-word">🎨 Определение</div>
                    <div class="top-word">«)" << stats.top_definition.first << R"(»</div>
                    <div class="top-count">)" << stats.top_definition.second << R"( раз(а)</div>
                </div>
                <div class="top-item">
                    <div class="top-word">📦 Дополнение</div>
                    <div class="top-word">«)" << stats.top_addition.first << R"(»</div>
                    <div class="top-count">)" << stats.top_addition.second << R"( раз(а)</div>
                </div>
                <div class="top-item">
                    <div class="top-word">📍 Обстоятельство</div>
                    <div class="top-word">«)" << stats.top_adverbial.first << R"(»</div>
                    <div class="top-count">)" << stats.top_adverbial.second << R"( раз(а)</div>
                </div>
            </div>
        </div>
    </div>
    
    <script>
        // Данные для круговой диаграммы членов предложения
        const membersCtx = document.getElementById('membersChart').getContext('2d');
        new Chart(membersCtx, {
            type: 'pie',
            data: {
                labels: [
                    'Подлежащие (' + )" << stats.subjects_total << R"( + ')',
                    'Сказуемые (' + )" << stats.predicates_total << R"( + ')',
                    'Определения (' + )" << stats.definitions_total << R"( + ')',
                    'Дополнения (' + )" << stats.additions_total << R"( + ')',
                    'Обстоятельства (' + )" << stats.adverbials_total << R"( + ')'
                ],
                datasets: [{
                    data: [)" << stats.subjects_total << ", " << stats.predicates_total << ", " 
                           << stats.definitions_total << ", " << stats.additions_total << ", " 
                           << stats.adverbials_total << R"(],
                    backgroundColor: [
                        '#FF6B6B',
                        '#4ECDC4',
                        '#45B7D1',
                        '#96CEB4',
                        '#FFEAA7'
                    ],
                    borderWidth: 2,
                    borderColor: '#fff'
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            font: { size: 12 }
                        }
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                const label = context.label || '';
                                const value = context.parsed || 0;
                                const total = context.dataset.data.reduce((a, b) => a + b, 0);
                                const percent = ((value / total) * 100).toFixed(1);
                                return `${label}: ${value} (${percent}%)`;
                            }
                        }
                    }
                }
            }
        });
        
        // Данные для круговой диаграммы популярных элементов (по частотности)
        const topCtx = document.getElementById('topElementsChart').getContext('2d');
        new Chart(topCtx, {
            type: 'doughnut',
            data: {
                labels: [
                    'Подлежащее: «)" << stats.top_subject.first << R"(»',
                    'Сказуемое: «)" << stats.top_predicate.first << R"(»',
                    'Определение: «)" << stats.top_definition.first << R"(»',
                    'Дополнение: «)" << stats.top_addition.first << R"(»',
                    'Обстоятельство: «)" << stats.top_adverbial.first << R"(»'
                ],
                datasets: [{
                    data: [)" << stats.top_subject.second << ", " << stats.top_predicate.second << ", "
                           << stats.top_definition.second << ", " << stats.top_addition.second << ", "
                           << stats.top_adverbial.second << R"(],
                    backgroundColor: [
                        '#FF6B6B',
                        '#4ECDC4',
                        '#45B7D1',
                        '#96CEB4',
                        '#FFEAA7'
                    ],
                    borderWidth: 2,
                    borderColor: '#fff'
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: true,
                plugins: {
                    legend: {
                        position: 'bottom',
                        labels: {
                            font: { size: 11 }
                        }
                    },
                    tooltip: {
                        callbacks: {
                            label: function(context) {
                                const label = context.label || '';
                                const value = context.parsed || 0;
                                return `${label}: ${value} раз(а)`;
                            }
                        }
                    }
                }
            }
        });
    </script>
</body>
</html>
)";
    
    return html.str();
}
*/
std::string generateHTMLCharts(const GlobalStats& stats) {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html lang=\"ru\">\n";
    html << "<head>\n";
    html << "    <meta charset=\"UTF-8\">\n";
    html << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "    <title>Статистика синтаксического анализа текста</title>\n";
    html << "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    html << "    <style>\n";
    html << "        body {\n";
    html << "            font-family: 'Segoe UI', Arial, sans-serif;\n";
    html << "            margin: 20px;\n";
    html << "            background-color: #f5f5f5;\n";
    html << "        }\n";
    html << "        .container {\n";
    html << "            max-width: 1200px;\n";
    html << "            margin: 0 auto;\n";
    html << "        }\n";
    html << "        h1 {\n";
    html << "            color: #333;\n";
    html << "            text-align: center;\n";
    html << "            font-weight: normal;\n";
    html << "        }\n";
    html << "        .stats-summary {\n";
    html << "            display: grid;\n";
    html << "            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));\n";
    html << "            gap: 20px;\n";
    html << "            margin-bottom: 30px;\n";
    html << "        }\n";
    html << "        .stat-card {\n";
    html << "            background: white;\n";
    html << "            border: 1px solid #ddd;\n";
    html << "            border-radius: 4px;\n";
    html << "            padding: 20px;\n";
    html << "            text-align: center;\n";
    html << "        }\n";
    html << "        .stat-value {\n";
    html << "            font-size: 32px;\n";
    html << "            font-weight: bold;\n";
    html << "            color: #333;\n";
    html << "        }\n";
    html << "        .stat-label {\n";
    html << "            color: #666;\n";
    html << "            margin-top: 5px;\n";
    html << "        }\n";
    html << "        .charts-grid {\n";
    html << "            display: grid;\n";
    html << "            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));\n";
    html << "            gap: 20px;\n";
    html << "            margin-bottom: 30px;\n";
    html << "        }\n";
    html << "        .chart-card {\n";
    html << "            background: white;\n";
    html << "            border: 1px solid #ddd;\n";
    html << "            border-radius: 4px;\n";
    html << "            padding: 20px;\n";
    html << "        }\n";
    html << "        .chart-card h3 {\n";
    html << "            text-align: center;\n";
    html << "            color: #333;\n";
    html << "            margin-bottom: 20px;\n";
    html << "            font-weight: normal;\n";
    html << "        }\n";
    html << "        canvas {\n";
    html << "            max-height: 300px;\n";
    html << "            margin: 0 auto;\n";
    html << "        }\n";
    html << "        .top-items {\n";
    html << "            background: white;\n";
    html << "            border: 1px solid #ddd;\n";
    html << "            border-radius: 4px;\n";
    html << "            padding: 20px;\n";
    html << "            margin-top: 20px;\n";
    html << "        }\n";
    html << "        .top-items h3 {\n";
    html << "            text-align: center;\n";
    html << "            color: #333;\n";
    html << "            margin-bottom: 20px;\n";
    html << "            font-weight: normal;\n";
    html << "        }\n";
    html << "        .top-grid {\n";
    html << "            display: grid;\n";
    html << "            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));\n";
    html << "            gap: 15px;\n";
    html << "        }\n";
    html << "        .top-item {\n";
    html << "            background-color: #f9f9f9;\n";
    html << "            border-radius: 4px;\n";
    html << "            padding: 12px;\n";
    html << "            text-align: center;\n";
    html << "            border-left: 3px solid #999;\n";
    html << "        }\n";
    html << "        .top-item .part {\n";
    html << "            font-size: 14px;\n";
    html << "            color: #666;\n";
    html << "            margin-bottom: 6px;\n";
    html << "        }\n";
    html << "        .top-item .word {\n";
    html << "            font-weight: bold;\n";
    html << "            font-size: 18px;\n";
    html << "            color: #333;\n";
    html << "            margin: 5px 0;\n";
    html << "        }\n";
    html << "        .top-item .count {\n";
    html << "            font-size: 20px;\n";
    html << "            font-weight: bold;\n";
    html << "            color: #333;\n";
    html << "        }\n";
    html << "        .footer {\n";
    html << "            text-align: center;\n";
    html << "            margin-top: 30px;\n";
    html << "            color: #999;\n";
    html << "            font-size: 12px;\n";
    html << "        }\n";
    html << "    </style>\n";
    html << "</head>\n";
    html << "<body>\n";
    html << "<div class=\"container\">\n";
    html << "    <h1>Статистика синтаксического анализа текста</h1>\n";
    html << "    \n";
    html << "    <div class=\"stats-summary\">\n";
    html << "        <div class=\"stat-card\">\n";
    html << "            <div class=\"stat-value\">" << stats.sentences_total << "</div>\n";
    html << "            <div class=\"stat-label\">Предложений</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-card\">\n";
    html << "            <div class=\"stat-value\">" << stats.words_total << "</div>\n";
    html << "            <div class=\"stat-label\">Слов</div>\n";
    html << "        </div>\n";
    html << "        <div class=\"stat-card\">\n";
    html << "            <div class=\"stat-value\">" << stats.members_total << "</div>\n";
    html << "            <div class=\"stat-label\">Членов предложения</div>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    \n";
    html << "    <div class=\"charts-grid\">\n";
    html << "        <div class=\"chart-card\">\n";
    html << "            <h3>Распределение членов предложения</h3>\n";
    html << "            <canvas id=\"membersChart\"></canvas>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    \n";
    html << "    <div class=\"top-items\">\n";
    html << "        <h3>Самые популярные слова</h3>\n";
    html << "        <div class=\"top-grid\">\n";
    html << "            <div class=\"top-item\">\n";
    html << "                <div class=\"part\">Подлежащее</div>\n";
    html << "                <div class=\"word\">«" << stats.top_subject.first << "»</div>\n";
    html << "                <div class=\"count\">" << stats.top_subject.second << " раз(а)</div>\n";
    html << "            </div>\n";
    html << "            <div class=\"top-item\">\n";
    html << "                <div class=\"part\">Сказуемое</div>\n";
    html << "                <div class=\"word\">«" << stats.top_predicate.first << "»</div>\n";
    html << "                <div class=\"count\">" << stats.top_predicate.second << " раз(а)</div>\n";
    html << "            </div>\n";
    html << "            <div class=\"top-item\">\n";
    html << "                <div class=\"part\">Определение</div>\n";
    html << "                <div class=\"word\">«" << stats.top_definition.first << "»</div>\n";
    html << "                <div class=\"count\">" << stats.top_definition.second << " раз(а)</div>\n";
    html << "            </div>\n";
    html << "            <div class=\"top-item\">\n";
    html << "                <div class=\"part\">Дополнение</div>\n";
    html << "                <div class=\"word\">«" << stats.top_addition.first << "»</div>\n";
    html << "                <div class=\"count\">" << stats.top_addition.second << " раз(а)</div>\n";
    html << "            </div>\n";
    html << "            <div class=\"top-item\">\n";
    html << "                <div class=\"part\">Обстоятельство</div>\n";
    html << "                <div class=\"word\">«" << stats.top_adverbial.first << "»</div>\n";
    html << "                <div class=\"count\">" << stats.top_adverbial.second << " раз(а)</div>\n";
    html << "            </div>\n";
    html << "        </div>\n";
    html << "    </div>\n";
    html << "    \n";
    html << "    <div class=\"footer\">\n";
    html << "        Giga Голиков AI\n";
    html << "    </div>\n";
    html << "</div>\n";
    html << "\n";
    html << "<script>\n";
    html << "    const membersCtx = document.getElementById('membersChart').getContext('2d');\n";
    html << "    new Chart(membersCtx, {\n";
    html << "        type: 'pie',\n";
    html << "        data: {\n";
    html << "            labels: [\n";
    html << "                'Подлежащие (" << stats.subjects_total << ")',\n";
    html << "                'Сказуемые (" << stats.predicates_total << ")',\n";
    html << "                'Определения (" << stats.definitions_total << ")',\n";
    html << "                'Дополнения (" << stats.additions_total << ")',\n";
    html << "                'Обстоятельства (" << stats.adverbials_total << ")'\n";
    html << "            ],\n";
    html << "            datasets: [{\n";
    html << "                data: [" << stats.subjects_total << ", " << stats.predicates_total << ", " 
         << stats.definitions_total << ", " << stats.additions_total << ", " << stats.adverbials_total << "],\n";
    html << "                backgroundColor: ['#4ECDC4', '#45B7D1', '#96CEB4', '#FFEAA7', '#FF6B6B'],\n";
    html << "                borderWidth: 1,\n";
    html << "                borderColor: '#fff'\n";
    html << "            }]\n";
    html << "        },\n";
    html << "        options: {\n";
    html << "            responsive: true,\n";
    html << "            maintainAspectRatio: true,\n";
    html << "            plugins: {\n";
    html << "                legend: { position: 'bottom' },\n";
    html << "                tooltip: {\n";
    html << "                    callbacks: {\n";
    html << "                        label: function(context) {\n";
    html << "                            const label = context.label || '';\n";
    html << "                            const value = context.parsed || 0;\n";
    html << "                            const total = context.dataset.data.reduce((a, b) => a + b, 0);\n";
    html << "                            const percent = ((value / total) * 100).toFixed(1);\n";
    html << "                            return label + ': ' + value + ' (' + percent + '%)';\n";
    html << "                        }\n";
    html << "                    }\n";
    html << "                }\n";
    html << "            }\n";
    html << "        }\n";
    html << "    });\n";
    html << "</script>\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}
  
// Пример использования
int main() {
    // Настройка консоли для корректного отображения русских символов
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    std::vector<SearchItem> searchItems = {
    // 1. Подлежащее
    {"кот", "подлежащее", 
     {"Пушистый кот спит на диване.", "Кот ловит мышь.", "Мой кот очень умный."}, 
     3},
    
    // 2. Сказуемое
    {"бежит", "сказуемое", 
     {"Спортсмен быстро бежит по стадиону.", "Кот бежит за мышью."}, 
     2},
    
    // 3. Дополнение
    {"книгу", "дополнение", 
     {"Я читаю интересную книгу.", "Он купил новую книгу.", "Дай мне эту книгу."}, 
     3},
    
    // 4. Определение
    {"красивый", "определение", 
     {"Красивый закат над морем.", "Это очень красивый цветок."}, 
     2},
    
    // 5. Обстоятельство
    {"быстро", "обстоятельство", 
     {"Машина быстро едет по трассе.", "Он быстро решил задачу.", "Время быстро летит."}, 
     3},
    
    // 6. Подлежащее
    {"солнце", "подлежащее", 
     {"Яркое солнце светит утром.", "Солнце заходит за горизонт."}, 
     2},
    
    // 7. Сказуемое
    {"работает", "сказуемое", 
     {"Программист работает за компьютером.", "Она работает в офисе.", "Завод работает круглосуточно."}, 
     3},
    
    // 8. Дополнение
    {"чай", "дополнение", 
     {"Я люблю пить горячий чай.", "Она налила чай в чашку."}, 
     2},
    
    // 9. Определение
    {"зимний", "определение", 
     {"Наступил холодный зимний вечер.", "Мы ждем зимний отпуск.", "Зимний пейзаж завораживает."}, 
     3},
    
    // 10. Обстоятельство
    {"вчера", "обстоятельство", 
     {"Вчера был дождливый день.", "Мы встречались вчера вечером."}, 
     2}
};
    // Создаем HTML страницу с расширенным поиском
    saveHTMLWithAdvancedSearch(searchItems, "search.html");
    std::cout << "Откройте search.html в браузере" << std::endl;
    
    GlobalStats stats;
    
    // Ваши реальные данные
    stats.sentences_total = 45;
    stats.words_total = 732;
    stats.members_total = 580;
    stats.subjects_total = 112;
    stats.predicates_total = 98;
    stats.definitions_total = 145;
    stats.additions_total = 103;
    stats.adverbials_total = 122;
    
    stats.top_subject = {"солнце", 15};
    stats.top_predicate = {"стоит", 8};
    stats.top_definition = {"яркий", 7};
    stats.top_addition = {"лес", 12};
    stats.top_adverbial = {"тихо", 9};

        // Генерируем HTML
    std::string html = generateHTMLCharts(stats);
    
    // Сохраняем в файл
    std::ofstream file("statistics.html");
    file << html;
    file.close();
    
    std::cout << "HTML файл создан: statistics.html" << std::endl;
    
    return 0;
}
  


/*
int main() {

    GlobalStats stats;
    
    // Ваши реальные данные
    stats.sentences_total = 45;
    stats.words_total = 732;
    stats.members_total = 580;
    stats.subjects_total = 112;
    stats.predicates_total = 98;
    stats.definitions_total = 145;
    stats.additions_total = 103;
    stats.adverbials_total = 122;
    
    stats.top_subject = {"солнце", 15};
    stats.top_predicate = {"стоит", 8};
    stats.top_definition = {"яркий", 7};
    stats.top_addition = {"лес", 12};
    stats.top_adverbial = {"тихо", 9};

        // Генерируем HTML
    std::string html = generateHTMLCharts(stats);
    
    // Сохраняем в файл
    std::ofstream file("statistics.html");
    file << html;
    file.close();
    
    std::cout << "HTML файл создан: statistics.html" << std::endl;
    
    return 0;
}
*/