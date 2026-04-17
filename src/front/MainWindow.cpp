#include "MainWindow.hpp"
#include <QCloseEvent>
#include <QCoreApplication>
#include <QFile>
#include <QFileDialog>
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
#include "../back/save_result.hpp"
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

void MainWindow::closeEvent(QCloseEvent *event) {
  // Проверяем, есть ли несохраненные результаты анализа
  if (!results.empty() && !hasUnsavedResults) {
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Подтверждение");
    msgBox.setText(
        "Результаты анализа не сохранены. Хотите сохранить их перед выходом?");
    msgBox.setIcon(QMessageBox::Question);
    QPushButton *yesButton = msgBox.addButton("Да", QMessageBox::YesRole);
    QPushButton *noButton = msgBox.addButton("Нет", QMessageBox::NoRole);
    QPushButton *cancelButton =
        msgBox.addButton("Отмена", QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == cancelButton) {
      event->ignore(); // Отменяем закрытие
      return;
    }

    if (msgBox.clickedButton() == yesButton) {
      // Пользователь хочет сохранить - открываем диалог сохранения
      QString path = QFileDialog::getExistingDirectory(
          this, "Выберите директорию для сохранения", QDir::homePath(),
          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

      if (path.isEmpty()) {
        // Пользователь отменил выбор директории - отменяем закрытие
        event->ignore();
        return;
      }

      // Проверка на наличие кириллицы в пути
      QRegularExpression cyrillicPattern("[\\u0400-\\u04FF]");
      if (cyrillicPattern.isValid() && cyrillicPattern.match(path).hasMatch()) {
        QMessageBox::warning(
            this, "Предупреждение",
            "Путь к директории содержит кириллические символы:\n" + path +
                "\n\nЭто может вызвать проблемы с сохранением файлов.");

        QMessageBox confirmMsgBox(this);
        confirmMsgBox.setWindowTitle("Подтверждение");
        confirmMsgBox.setText("Вы действительно хотите продолжить сохранение?");
        confirmMsgBox.setIcon(QMessageBox::Question);
        QPushButton *confirmYesButton =
            confirmMsgBox.addButton("Да", QMessageBox::YesRole);
        QPushButton *confirmNoButton =
            confirmMsgBox.addButton("Нет", QMessageBox::NoRole);
        confirmMsgBox.exec();

        if (confirmMsgBox.clickedButton() != confirmYesButton) {
          event->ignore(); // Отменяем закрытие
          return;
        }
      }

      std::string stdPath = path.toStdString();

      const std::string SEARCH_FILE = "list.html";
      const std::string REVIEW_FILE = "statistics.html";

      QString searchFilePath = path + "/" + QString::fromStdString(SEARCH_FILE);
      QString reviewFilePath = path + "/" + QString::fromStdString(REVIEW_FILE);

      // Проверяем, существуют ли уже файлы с таким именем
      bool searchFileExists = QFile::exists(searchFilePath);
      bool reviewFileExists = QFile::exists(reviewFilePath);

      if (searchFileExists || reviewFileExists) {
        QMessageBox warningMsgBox(this);
        warningMsgBox.setWindowTitle("Предупреждение");
        QString warningText = "В выбранной директории уже существуют файлы:\n";
        if (searchFileExists) {
          warningText += "- " + SEARCH_FILE + "\n";
        }
        if (reviewFileExists) {
          warningText += "- " + REVIEW_FILE + "\n";
        }
        warningText += "\nЕсли вы продолжите, эти файлы будут перезаписаны.";
        warningMsgBox.setText(warningText);
        warningMsgBox.setIcon(QMessageBox::Warning);
        QPushButton *overwriteButton =
            warningMsgBox.addButton("Продолжить", QMessageBox::YesRole);
        QPushButton *cancelOverwriteButton =
            warningMsgBox.addButton("Отмена", QMessageBox::RejectRole);
        warningMsgBox.exec();

        if (warningMsgBox.clickedButton() != overwriteButton) {
          event->ignore(); // Отменяем закрытие
          return;
        }
      }

      // Сохраняем результаты
      std::vector<SearchItem> items = build_search_items(results);
      GlobalStats statistics = build_global_stats(results);
      saveAnalysis(stdPath, items, statistics);

      // Проверяем, что файлы сохранились
      if (QFile::exists(searchFilePath) && QFile::exists(reviewFilePath)) {
        QMessageBox::information(this, "Сохранение",
                                 "Результаты успешно сохранены в:\n" + path);
      } else {
        QMessageBox::critical(
            this, "Ошибка сохранения",
            "Не удалось сохранить файлы результатов анализа.");
      }
    }
  }

  event->accept(); // Разрешаем закрытие
}

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

  // Устанавливаем флаг, что есть результаты, но они ещё не сохранены
  hasUnsavedResults = true;

  stackedWidget->setCurrentWidget(resultPage);
}

void MainWindow::onSearchRequested() {
  stackedWidget->setCurrentWidget(searchPage);
}

void MainWindow::onNewAnalysisRequested() {
  results.clear();
  hasUnsavedResults = false; // Сбрасываем флаг при начале нового анализа
  stackedWidget->setCurrentWidget(inputPage);
}

void MainWindow::onBackToResultRequested() {
  // Обновляем данные перед возвратом
  resultPage->buildWordRoleMap();
  resultPage->updateCounts();
  resultPage->updateChart();

  stackedWidget->setCurrentWidget(resultPage);
}
