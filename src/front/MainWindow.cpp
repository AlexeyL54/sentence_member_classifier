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

/**
 * @brief Конструктор главного окна
 * @param parent Родительский виджет
 * @details Инициализирует пользовательский интерфейс, настраивает
 * сигнально-слотовые соединения и загружает необходимые модели
 */
MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Анализатор текста");
  resize(1200, 700);

  setupUI();
  setupConnections();
  initializeModels();
}

/**
 * @brief Деструктор главного окна
 */
MainWindow::~MainWindow() {}

/**
 * @brief Обработчик события закрытия окна
 * @param event Событие закрытия
 * @details Проверяет наличие несохранённых результатов и предлагает их
 * сохранить
 */
void MainWindow::closeEvent(QCloseEvent *event) {
  if (results.empty() || !hasUnsavedResults) {
    event->accept();
    return;
  }

  if (saveResultsOnClose()) {
    event->accept();
  } else {
    event->ignore();
  }
}

/**
 * @brief Слот для обработки запроса на анализ текста
 * @param text Текст для анализа
 * @details Выполняет проверку наличия модели и запускает процесс анализа
 */
void MainWindow::onAnalyzeRequested(const std::string &text) {
  if (!inferer) {
    showParsingError();
    return;
  }
  processAnalysis(text);
}

/**
 * @brief Слот для обработки запроса на поиск
 * @details Переключает стековый виджет на страницу поиска
 */
void MainWindow::onSearchRequested() {
  stackedWidget->setCurrentWidget(searchPage);
}

/**
 * @brief Слот для запроса нового анализа
 * @details Очищает результаты анализа и переключается на страницу ввода текста
 */
void MainWindow::onNewAnalysisRequested() {
  results.clear();
  hasUnsavedResults = false;
  stackedWidget->setCurrentWidget(inputPage);
}

/**
 * @brief Слот для возврата к странице результатов
 * @details Обновляет отображение счетчиков и диаграммы, переключается на
 * страницу результатов
 */
void MainWindow::onBackToResultRequested() {
  resultPage->updateCounts();
  resultPage->updateChart();
  stackedWidget->setCurrentWidget(resultPage);
}

/**
 * @brief Настройка пользовательского интерфейса
 * @details Создает и настраивает все страницы, добавляет их в стековый виджет
 */
void MainWindow::setupUI() {
  stackedWidget = new QStackedWidget(this);

  inputPage = new InputPage(this);
  resultPage = new ResultPage(this);
  searchPage = new SearchPage(this);
  loadingPage = new LoadingPage(this);

  stackedWidget->addWidget(inputPage);   // индекс 0
  stackedWidget->addWidget(resultPage);  // индекс 1
  stackedWidget->addWidget(searchPage);  // индекс 2
  stackedWidget->addWidget(loadingPage); // индекс 3

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(stackedWidget);

  stackedWidget->setCurrentWidget(inputPage);
}

/**
 * @brief Настройка сигнально-слотовых соединений
 * @details Устанавливает соединения между сигналами страниц и слотами главного
 * окна
 */
void MainWindow::setupConnections() {
  connect(inputPage, &InputPage::analysisRequested, this,
          &MainWindow::onAnalyzeRequested);

  connect(resultPage, &ResultPage::searchRequested, this,
          &MainWindow::onSearchRequested);

  connect(resultPage, &ResultPage::newAnalysisRequested, this,
          &MainWindow::onNewAnalysisRequested);

  connect(searchPage, &SearchPage::backRequested, this,
          &MainWindow::onBackToResultRequested);
}

/**
 * @brief Инициализация всех моделей (обёртка)
 * @details Вызывает методы загрузки меток, токенизатора и модели
 */
void MainWindow::initializeModels() {
  setLabels();
  setTokenizer();
  setModel();
  inferer = std::make_unique<BertOnnxInference>(std::move(model), tokenizer,
                                                labels, 128);
}

/**
 * @brief Проверка наличия необходимых директорий
 * @details Проверяет существование директории ../model, при отсутствии
 * показывает ошибку
 */
void MainWindow::checkAppDirs() {
  if (!std::filesystem::exists("../model")) {
    QMessageBox::warning(this, "Ошибка", "Директория ../model не существует!");
    close();
  }
}

/**
 * @brief Инициализация токенизатора
 * @details Создает токенизатор на основе файла словаря
 */
void MainWindow::setTokenizer() {
  try {
    tokenizer = std::make_shared<SimpleTokenizer>("../model/vocab.txt");
  } catch (std::exception &e) {
    QMessageBox::warning(this, "Ошибка", "Не удалось загрузить словарь!");
    close();
  }
}

/**
 * @brief Загрузка меток из конфигурации
 * @details Загружает метки сущностей из файла конфигурации модели
 */
void MainWindow::setLabels() {
  try {
    labels = load_labels("../model/config.json");
  } catch (const std::exception &) {
    QMessageBox::warning(this, "Ошибка",
                         "Не удалось загрузить конфигурацию модели!");
    close();
  }
}

/**
 * @brief Загрузка ONNX модели
 * @details Загружает модель BERT NER из файла
 */
void MainWindow::setModel() {
  try {
    model = std::make_unique<onnx_infer::BertNerModel>(
        "../model/bert_ner_model.onnx");
  } catch (std::exception &e) {
    QMessageBox::warning(this, "Ошибка", "Не удалось загрузить модель!");
    close();
  }
}

/**
 * @brief Отображение ошибки парсинга
 * @details Показывает сообщение об ошибке и возвращается на страницу ввода
 */
void MainWindow::showParsingError() {
  QMessageBox::warning(this, "Ошибка обработки текста",
                       "Текст содержит неопределенные символы!");
  stackedWidget->setCurrentWidget(inputPage);
  results.clear();
}

/**
 * @brief Обновить вывод результатов анализа текста
 * @details Строит элементы поиска и статистику, обновляет страницы результатов
 */
void MainWindow::updatePagesWithResults() {
  std::vector<SearchItem> items = build_search_items(results);
  GlobalStats statistics = build_global_stats(results);

  searchPage->setSearchItems(items);
  resultPage->setSearchItems(items);
  resultPage->setGlobalStats(statistics);
  resultPage->setData(results);
  resultPage->updateCounts();
  resultPage->updateChart();
  resultPage->updateStatsDisplay();

  hasUnsavedResults = true;
  stackedWidget->setCurrentWidget(resultPage);
}

/**
 * @brief Запуск процесса анализа текста
 * @param text Текст для анализа
 * @details Разбивает текст на предложения и обрабатывает каждое с обновлением
 * прогресса
 */
void MainWindow::processAnalysis(const std::string &text) {
  stackedWidget->setCurrentWidget(loadingPage);

  std::vector<std::string> sentences = inferer->split_into_sentences(text);
  int totalSentences = static_cast<int>(sentences.size());
  loadingPage->setTotal(totalSentences);
  loadingPage->reset();

  results.clear();
  int processedCount = 0;

  for (const std::string &sentence : sentences) {
    SentenceResult result = inferer->process_sentence(sentence);
    results.push_back(result);

    if (result.err == 1) {
      showParsingError();
      return;
    }

    processedCount++;
    loadingPage->setProgress(processedCount);
    QCoreApplication::processEvents();
  }
  updatePagesWithResults();
}

/**
 * @brief Проверка пути на наличие кириллицы
 * @param path Путь для проверки
 * @return true если путь содержит кириллические символы
 */
bool MainWindow::hasCyrillicPath(const QString &path) const {
  for (const QChar &ch : path) {
    if (ch.unicode() >= 0x0400 && ch.unicode() <= 0x04FF) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Запрос подтверждения перезаписи существующих файлов
 * @param searchFilePath Путь к файлу поиска
 * @param reviewFilePath Путь к файлу обзора
 * @return true если пользователь подтвердил перезапись
 */
bool MainWindow::confirmFileOverwrite(const QString &searchFilePath,
                                      const QString &reviewFilePath) {
  bool searchExists = QFile::exists(searchFilePath);
  bool reviewExists = QFile::exists(reviewFilePath);

  if (!searchExists && !reviewExists) {
    return true;
  }

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Предупреждение");
  QString warningText = "В выбранной директории уже существуют файлы:\n";

  if (searchExists) {
    warningText += "- list.html\n";
  }
  if (reviewExists) {
    warningText += "- statistics.html\n";
  }
  warningText += "\nЕсли вы продолжите, эти файлы будут перезаписаны.";

  msgBox.setText(warningText);
  msgBox.setIcon(QMessageBox::Warning);

  QPushButton *overwriteButton =
      msgBox.addButton("Продолжить", QMessageBox::YesRole);
  msgBox.addButton("Отмена", QMessageBox::RejectRole);
  msgBox.exec();

  return msgBox.clickedButton() == overwriteButton;
}

/**
 * @brief Сохранение файлов результатов
 * @param directoryPath Путь для сохранения
 * @return true если сохранение успешно
 */
bool MainWindow::saveResultFiles(const QString &directoryPath) {
  const std::string SEARCH_FILE = "list.html";
  const std::string REVIEW_FILE = "statistics.html";

  QString searchFilePath =
      directoryPath + "/" + QString::fromStdString(SEARCH_FILE);
  QString reviewFilePath =
      directoryPath + "/" + QString::fromStdString(REVIEW_FILE);

  if (!confirmFileOverwrite(searchFilePath, reviewFilePath)) {
    return false;
  }

  std::vector<SearchItem> items = build_search_items(results);
  GlobalStats statistics = build_global_stats(results);

  saveAnalysis(directoryPath.toStdString(), items, statistics);

  if (QFile::exists(searchFilePath) && QFile::exists(reviewFilePath)) {
    QMessageBox::information(
        this, "Сохранение",
        "Результаты успешно сохранены в:\n" + directoryPath + "\n" + "- " +
            QString::fromStdString(SEARCH_FILE) + "\n" + "- " +
            QString::fromStdString(REVIEW_FILE) + "\n");
    return true;
  } else {
    QMessageBox::critical(this, "Ошибка сохранения",
                          "Не удалось сохранить файлы результатов анализа.");
    return false;
  }
}

/**
 * @brief Запрос подтверждения при обнаружении кириллицы в пути
 * @param path Путь, содержащий кириллицу
 * @return true если пользователь подтвердил продолжение, false если отменил
 */
bool MainWindow::confirmCyrillicPath(const QString &path) {
  QMessageBox::warning(
      this, "Предупреждение",
      "Путь к директории содержит кириллические символы:\n" + path +
          "\n\nЭто может вызвать проблемы с сохранением файлов.");

  QMessageBox confirmMsgBox(this);
  confirmMsgBox.setWindowTitle("Подтверждение");
  confirmMsgBox.setText("Вы действительно хотите продолжить сохранение?");
  confirmMsgBox.setIcon(QMessageBox::Question);

  QPushButton *confirmYes = confirmMsgBox.addButton("Да", QMessageBox::YesRole);
  confirmMsgBox.addButton("Нет", QMessageBox::NoRole);
  confirmMsgBox.exec();

  return confirmMsgBox.clickedButton() == confirmYes;
}

/**
 * @brief Сохранение результатов анализа при закрытии окна
 * @return true если сохранение прошло успешно или не требуется, false если
 * отменено
 */
bool MainWindow::saveResultsOnClose() {
  if (resultPage->getSaveStatus()) {
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
      return false;
    }

    if (msgBox.clickedButton() == noButton) {
      return true;
    }

    // Обработка сохранения
    QString path = QFileDialog::getExistingDirectory(
        this, "Выберите директорию для сохранения", QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (path.isEmpty()) {
      return false;
    }

    if (hasCyrillicPath(path) && !confirmCyrillicPath(path)) {
      return false;
    }

    return saveResultFiles(path);
  }
  return true;
}
