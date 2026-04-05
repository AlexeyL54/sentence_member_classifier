#include "MainWindow.hpp"
#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>
#include <iostream>
#include <memory>
#include <ostream>
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
      load_labels("../model/config.json");

  auto tokenizer = std::make_shared<SimpleTokenizer>("../model/vocab.txt");

  std::unique_ptr<onnx_infer::BertNerModel> model =
      std::make_unique<onnx_infer::BertNerModel>(
          "../model/bert_ner_model.onnx");

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

  // Связываем сигналы SearchPage
  connect(searchPage, &SearchPage::backRequested, this,
          &MainWindow::onBackToResultRequested);
}

void MainWindow::onAnalyzeRequested(const std::string &text) {
  std::cout << "[DEBUG] onAnalyzeRequested called" << std::endl;

  if (!inferer) {
    std::cerr << "[ERROR] inferer is null!" << std::endl;
    return;
  }

  std::cout << "[DEBUG] Calling extract_sentence_parts..." << std::endl;
  results = inferer->extract_sentence_parts(text);
  std::cout << "[DEBUG] Got " << results.size() << " sentences" << std::endl;

  std::vector<SearchItem> items = build_search_items(results);

  std::cout << "Search Items:" << std::endl;
  for (const SearchItem &item : items) {
    std::cout << item.text << std::endl;
    std::cout << item.type << std::endl;
    std::cout << item.amount << std::endl;
    std::cout << std::endl;
  }

  searchPage->setSearchItems(items);
  resultPage->setData(results);
  resultPage->updateCounts();
  resultPage->updateChart();

  // Вместо этого пока просто выводим результат в консоль
  for (const auto &sent : results) {
    std::cout << "Sentence: " << sent.text << std::endl;
    for (const auto &ent : sent.entities) {
      std::cout << "  " << ent.type_ru << ": " << ent.text << std::endl;
    }
  }

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
