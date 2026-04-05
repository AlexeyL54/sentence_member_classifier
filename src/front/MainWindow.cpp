#include "MainWindow.hpp"
#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>
#include <memory>
#include <vector>

#include "../back/bert_onnx_inference.hpp"
#include "../back/onnx_model.hpp"
#include "../back/simple_tokenizer.hpp"

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Анализатор текста");
  resize(1200, 700);

  setupUI();
  setupConnections();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {

  std::map<int, std::pair<std::string, std::string>> labels =
      load_labels("model/config.json");

  SimpleTokenizer tokenizer("model/vocab.txt");

  std::unique_ptr<onnx_infer::BertNerModel> model =
      std::make_unique<onnx_infer::BertNerModel>("model");

  inferer = std::make_unique<BertOnnxInference>(std::move(model), tokenizer,
                                                labels, 128);

  // Создаём стековый виджет для переключения страниц
  stackedWidget = new QStackedWidget(this);

  // Создаём страницы
  inputPage = new InputPage(this);
  resultPage = new ResultPage(this);

  // Загружаем данные в ResultPage из готовых файлов
  // Важно: порядок загрузки имеет значение!
  // resultPage->setData(resultPage->makeData());
  // resultPage->updateCounts(); // Обновляем счетчики
  // resultPage->updateChart();  // Обновляем диаграмму

  // Создаём SearchResultItem для страницы поиска из готовых данных

  std::vector<SearchItem> debugItems = {
      {"вчера",
       "обстоятельство",
       {"Вчера я ходил в школу и учился читать книгу."},
       2},
      {"я", "подлежащее", {"Вчера я ходил в школу и учился читать книгу."}, 3},
      {"ходил",
       "сказуемое",
       {"Вчера я ходил в школу и учился читать книгу."},
       1},
      {"школу",
       "дополнение",
       {"Вчера я ходил в школу и учился читать книгу."},
       1},
      {"учился",
       "сказуемое",
       {"Вчера я ходил в школу и учился читать книгу."},
       1},
      {"читать",
       "дополнение",
       {"Вчера я ходил в школу и учился читать книгу."},
       2},
      {"книгу",
       "дополнение",
       {"Вчера я ходил в школу и учился читать книгу."},
       2},
      {"сегодня", "обстоятельство", {"Сегодня я читаю книгу быстро."}, 1},
      {"читаю", "сказуемое", {"Сегодня я читаю книгу быстро."}, 1},
      {"быстро", "обстоятельство", {"Сегодня я читаю книгу быстро."}, 1},
      {"потому",
       "обстоятельство",
       {"Потому что я учился вчера, я буду продолжать читать."},
       1},
      {"если",
       "обстоятельство",
       {"Если я учился вчера, то я буду продолжать читать."},
       1},
      {"буду",
       "сказуемое",
       {"Потому что я учился вчера, я буду продолжать читать."},
       1},
      {"продолжать",
       "сказуемое",
       {"Потому что я учился вчера, я буду продолжать читать."},
       1},
      {"то",
       "обстоятельство",
       {"Если я учился вчера, то я буду продолжать читать."},
       1},
      {"когда",
       "обстоятельство",
       {"Сегодня я читаю книгу быстро, когда появляется время."},
       1}};
  // SearchPage window(debugItems);

  // searchPage = new SearchPage(searchItems_, this);
  searchPage = new SearchPage(debugItems, this);

  // Добавляем страницы в стек
  stackedWidget->addWidget(inputPage);  // индекс 0
  stackedWidget->addWidget(resultPage); // индекс 1
  stackedWidget->addWidget(searchPage); // индекс 2

  // Главный layout
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(stackedWidget);

  // Показываем страницу ввода
  stackedWidget->setCurrentWidget(inputPage);
}

void MainWindow::setupConnections() {
  // Связываем сигналы InputPage
  connect(inputPage, &InputPage::analysisRequested, this,
          &MainWindow::onAnalyzeRequested);

  // Связываем сигналы ResultPage
  connect(resultPage, &ResultPage::searchRequested, this,
          &MainWindow::onSearchRequested);

  // Связываем сигналы SearchPage
  connect(searchPage, &SearchPage::backRequested, this,
          &MainWindow::onBackToResultRequested);
}

void MainWindow::onAnalyzeRequested(const std::string &text) {
  std::vector<SentenceResult> result = inferer->extract_sentence_parts(text);
  resultPage->setData(resultPage->makeData());
  resultPage->updateCounts();
  resultPage->updateChart();

  // Переходим на страницу результатов
  stackedWidget->setCurrentWidget(resultPage);
}

void MainWindow::onSearchRequested() {
  // Переход на страницу поиска
  stackedWidget->setCurrentWidget(searchPage);
}

void MainWindow::onBackToResultRequested() {
  // Обновляем данные перед возвратом
  resultPage->buildWordRoleMap();
  resultPage->updateCounts();
  resultPage->updateChart();

  // Возврат на страницу результатов
  stackedWidget->setCurrentWidget(resultPage);
}
