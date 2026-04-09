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

  setLabels();
  setTokenizer();
  setModel();
  inferer = std::make_unique<BertOnnxInference>(std::move(model), tokenizer,
                                                labels, 128);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {

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

void MainWindow::ckeckAppDirs() {
  if (!std::filesystem::exists("../model")) {
    QMessageBox::warning(this, "Ошибка", "Директория ../model не существует!");
    this->close();
  }
}

void MainWindow::setTokenizer() {
  try {
    tokenizer = std::make_shared<SimpleTokenizer>(("../model/vocab.txt"));
  } catch (std::exception &e) {
    QMessageBox::warning(this, "Ошибка", "Не удалось загрузить словарь!");
    this->close();
  }
}

void MainWindow::setLabels() {
  try {
    labels = load_labels(("../model/config.json"));
  } catch (const std::exception &) {
    QMessageBox::warning(this, "Ошибка",
                         "Не удалось загрузить конфигурацию модели!");
    this->close();
  }
}

void MainWindow::setModel() {
  try {
    model = std::make_unique<onnx_infer::BertNerModel>(
        ("../model/bert_ner_model.onnx"));
  } catch (std::exception &e) {
    QMessageBox::warning(this, "Ошибка", "Не удалось загрузить модель!");
    this->close();
  }
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

  stackedWidget->setCurrentWidget(resultPage);
}

void MainWindow::onSearchRequested() {
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

  stackedWidget->setCurrentWidget(resultPage);
}
