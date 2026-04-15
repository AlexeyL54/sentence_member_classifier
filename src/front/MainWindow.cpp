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
#include "LoadingPage.hpp"

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
  loadingPage = new LoadingPage(this);

  // Добавляем страницы в стек
  stackedWidget->addWidget(inputPage);   // индекс 0
  stackedWidget->addWidget(resultPage);  // индекс 1
  stackedWidget->addWidget(searchPage);  // индекс 2
  stackedWidget->addWidget(loadingPage); // индекс 3

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
  // Показываем страницу загрузки и устанавливаем начальный прогресс
  stackedWidget->setCurrentWidget(loadingPage);

  // Разбиваем текст на предложения для подсчёта общего количества
  std::vector<std::string> sentences = inferer->split_into_sentences(text);
  int totalSentences = static_cast<int>(sentences.size());
  loadingPage->setTotal(totalSentences);
  loadingPage->reset();

  // Обрабатываем текст с обновлением прогресса
  results.clear();
  int processedCount = 0;

  for (const std::string &sentence : sentences) {
    // if (sentence.length() < 3)
    // continue; // Пропускаем слишком короткие

    SentenceResult result = inferer->process_sentence(sentence);
    results.push_back(result);

    processedCount++;
    loadingPage->setProgress(processedCount);

    // Даём интерфейсу время обновиться
    QCoreApplication::processEvents();
  }

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
