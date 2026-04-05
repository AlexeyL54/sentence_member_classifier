#include "MainWindow.hpp"
#include <QFile>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>
#include <QVBoxLayout>
#include <vector>

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
  resultPage->buildWordRoleMap(); // Строим карту соответствий слов
  resultPage->updateCounts();     // Обновляем счетчики
  resultPage->updateChart();      // Обновляем диаграмму
  // Не вызываем refreshDisplay(), так как buildWordRoleMap уже вызывает
  // setMarkupText

  // Создаём SearchResultItem для страницы поиска из готовых данных
  // createSearchItems();

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

void MainWindow::onAnalyzeRequested() {
  // Обновляем данные перед показом (на случай, если они изменились)
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
  resultPage->buildWordRoleMap();
  resultPage->updateCounts();
  resultPage->updateChart();

  // Возврат на страницу результатов
  stackedWidget->setCurrentWidget(resultPage);
}
