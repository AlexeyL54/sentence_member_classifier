#include "MainWindow.hpp"
#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Анализатор текста");
  resize(1200, 700);

  setupUI();
  setupConnections();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
  // Создаём стековый виджет для переключения страниц
  stackedWidget = new QStackedWidget(this);

  // Создаём страницы
  inputPage = new InputPage(this);
  resultPage = new ResultPage(this);

  // Загружаем данные в ResultPage из готовых файлов
  // Важно: порядок загрузки имеет значение!
  resultPage->readTextFromFile("text.txt"); // Сначала загружаем текст
  resultPage->loadParsedData("data.txt"); // Затем загружаем разобранные данные
  resultPage->buildWordRoleMap();         // Строим карту соответствий слов
  resultPage->updateCounts();             // Обновляем счетчики
  resultPage->updateChart();              // Обновляем диаграмму
  // Не вызываем refreshDisplay(), так как buildWordRoleMap уже вызывает
  // setMarkupText

  // Создаём SearchResultItem для страницы поиска из готовых данных
  // createSearchItems();

  QVector<SearchResultItem> debugItems = {
      {"вчера", "Обстоятельство", 1,
       "Вчера я ходил в школу и учился читать книгу.", 2},
      {"я", "Подлежащее", 1, "Вчера я ходил в школу и учился читать книгу.", 3},
      {"ходил", "Сказуемое", 1, "Вчера я ходил в школу и учился читать книгу.",
       1},
      {"школу", "Дополнение", 1, "Вчера я ходил в школу и учился читать книгу.",
       1},
      {"учился", "Сказуемое", 1, "Вчера я ходил в школу и учился читать книгу.",
       1},
      {"читать", "Дополнение", 1,
       "Вчера я ходил в школу и учился читать книгу.", 2},
      {"книгу", "Дополнение", 1, "Вчера я ходил в школу и учился читать книгу.",
       2},
      {"сегодня", "Обстоятельство", 2, "Сегодня я читаю книгу быстро.", 1},
      {"читаю", "Сказуемое", 2, "Сегодня я читаю книгу быстро.", 1},
      {"быстро", "Обстоятельство", 2, "Сегодня я читаю книгу быстро.", 1},
      {"потому", "Обстоятельство", 3,
       "Потому что я учился вчера, я буду продолжать читать.", 1},
      {"если", "Обстоятельство", 3,
       "Если я учился вчера, то я буду продолжать читать.", 1},
      {"буду", "Сказуемое", 3,
       "Потому что я учился вчера, я буду продолжать читать.", 1},
      {"продолжать", "Сказуемое", 3,
       "Потому что я учился вчера, я буду продолжать читать.", 1},
      {"то", "Обстоятельство", 3,
       "Если я учился вчера, то я буду продолжать читать.", 1},
      {"когда", "Обстоятельство", 2,
       "Сегодня я читаю книгу быстро, когда появляется время.", 1}};

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
  connect(inputPage, &InputPage::analyzeRequested, this,
          &MainWindow::onAnalyzeRequested);

  // Связываем сигналы ResultPage
  connect(resultPage, &ResultPage::searchRequested, this,
          &MainWindow::onSearchRequested);

  // Связываем сигналы SearchPage
  connect(searchPage, &SearchPage::backRequested, this,
          &MainWindow::onBackToResultRequested);
}

void MainWindow::createSearchItems() {
  searchItems_.clear();

  // Читаем текст
  QFile textFile("text.txt");
  if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  QTextStream in(&textFile);
  in.setEncoding(QStringConverter::Utf8);
  QString text = in.readAll();
  textFile.close();

  // Читаем данные о членах предложения
  QMap<QString, QString> wordToCategory;
  QFile dataFile("data.txt");
  if (dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream inData(&dataFile);
    inData.setEncoding(QStringConverter::Utf8);

    while (!inData.atEnd()) {
      QString line = inData.readLine().trimmed();
      if (line.isEmpty())
        continue;

      QStringList parts = line.split(": ");
      if (parts.size() != 2)
        continue;

      QString category = parts[0];
      QStringList wordsList = parts[1].split(", ", Qt::SkipEmptyParts);

      for (const QString &w : wordsList) {
        wordToCategory[w] = category;
      }
    }
    dataFile.close();
  }

  // Разбиваем текст на предложения
  QStringList sentences =
      text.split(QRegularExpression("[.!?]"), Qt::SkipEmptyParts);

  // Считаем частоту слов
  QRegularExpression re("\\w+");
  QMap<QString, int> wordCount;
  for (const QString &sentence : sentences) {
    QRegularExpressionMatchIterator wordIter = re.globalMatch(sentence);
    while (wordIter.hasNext()) {
      QRegularExpressionMatch match = wordIter.next();
      QString word = match.captured(0);
      wordCount[word] = wordCount.value(word, 0) + 1;
    }
  }

  // Создаём элементы для поиска
  for (int i = 0; i < sentences.size(); ++i) {
    QString sentence = sentences[i];
    QRegularExpressionMatchIterator wordIter = re.globalMatch(sentence);
    while (wordIter.hasNext()) {
      QRegularExpressionMatch match = wordIter.next();
      QString word = match.captured(0);

      SearchResultItem item;
      item.word = word;
      item.member = wordToCategory.value(word, "Другое");
      item.sentenceNo = i + 1;
      item.sentenceText = sentence.trimmed();
      item.count = wordCount.value(word, 1);

      searchItems_.append(item);
    }
  }
}

void MainWindow::onAnalyzeRequested() {
  // Обновляем данные перед показом (на случай, если они изменились)
  resultPage->readTextFromFile("text.txt");
  resultPage->loadParsedData("data.txt");
  resultPage->buildWordRoleMap();
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
  resultPage->readTextFromFile("text.txt");
  resultPage->loadParsedData("data.txt");
  resultPage->buildWordRoleMap();
  resultPage->updateCounts();
  resultPage->updateChart();

  // Возврат на страницу результатов
  stackedWidget->setCurrentWidget(resultPage);
}
