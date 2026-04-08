#include "MainWindow.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>
#include <exception>
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
  QString appDirPath = QCoreApplication::applicationDirPath();
  QString modelDir = appDirPath + "/model";

  std::map<int, std::pair<std::string, std::string>> labels;
  std::shared_ptr<SimpleTokenizer> tokenizer;
  std::unique_ptr<onnx_infer::BertNerModel> model;

  try {
    labels = load_labels((modelDir + "/config.json").toStdString());
    tokenizer = std::make_shared<SimpleTokenizer>(
        (modelDir + "/vocab.txt").toStdString());
    model = std::make_unique<onnx_infer::BertNerModel>(
        (modelDir + "/bert_ner_model.onnx").toStdString());

  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Ошибка",
                         "Не найдена директория, содержащая модель!");
  }

  inferer = std::make_unique<BertOnnxInference>(std::move(model), tokenizer,
                                                labels, 128);

  // Создаём стековый виджет для переключения страниц
  stackedWidget = new QStackedWidget(this);

  // Создаём страницы
  inputPage = new InputPage(this);
  resultPage = new ResultPage(this);

  searchPage = new SearchPage(this);

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

  connect(resultPage, &ResultPage::newAnalysisRequested, this,
          &MainWindow::onNewAnalysisRequested);

  // Связываем сигналы SearchPage
  connect(searchPage, &SearchPage::backRequested, this,
          &MainWindow::onBackToResultRequested);
}

void MainWindow::onAnalyzeRequested(const std::string &text) {
  results = inferer->extract_sentence_parts(text);

  std::vector<SearchItem> items = build_search_items(results);
  GlobalStats statistics = build_global_stats(results);

  searchPage->setSearchItems(items);
  resultPage->setSearchItems(items);
  resultPage->setGloabalStats(statistics);
  resultPage->setData(results);
  resultPage->updateCounts();
  resultPage->updateChart();
  resultPage->updateStatsDisplay();

  // Переходим на страницу результатов
  stackedWidget->setCurrentWidget(resultPage);
}

void MainWindow::onSearchRequested() {
  // Переход на страницу поиска
  stackedWidget->setCurrentWidget(searchPage);
}

void MainWindow::onNewAnalysisRequested() {
  results.clear();
  stackedWidget->setCurrentWidget(inputPage);
}

void MainWindow::onBackToResultRequested() {
  // Обновляем данные перед возвратом
  resultPage->buildWordRoleMap();
  resultPage->updateCounts();
  resultPage->updateChart();

  // Возврат на страницу результатов
  stackedWidget->setCurrentWidget(resultPage);
}
