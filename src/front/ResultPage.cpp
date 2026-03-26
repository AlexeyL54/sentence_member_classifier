#include "ResultPage.hpp"
#include "lib/TextMarkupWidget.hpp"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QVector>

/**
 * @brief Конструктор класса ResultPage.
 * @param parent Указатель на родительский виджет.
 */
ResultPage::ResultPage(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle("Анализ предложения");
  resize(1200, 400);

  // Создаем центральный виджет и главный layout
  QWidget *centralWidget = new QWidget(this);
  QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(10);
  setCentralWidget(centralWidget);

  // Левая часть: текстовое поле с разметкой
  leftWidget = new QWidget(centralWidget);
  leftWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
  leftLayout->setContentsMargins(10, 10, 10, 10);
  leftLayout->setSpacing(10);

  widgetText = new TextMarkupWidget(leftWidget);
  widgetText->setWordColor(QApplication::palette().text().color().name());
  widgetText->setLabelColor("#808080");
  widgetText->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  leftLayout->addWidget(widgetText, 1);

  // Кнопки внизу
  QWidget *buttonContainer = new QWidget(leftWidget);
  QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);
  buttonLayout->setContentsMargins(0, 0, 0, 0);

  btnSave = new QPushButton("Сохранить", buttonContainer);
  btnSearch = new QPushButton("Поиск", buttonContainer);

  buttonLayout->addWidget(btnSave);
  buttonLayout->addWidget(btnSearch);
  buttonLayout->addStretch();

  btnSave->setFixedWidth(150);
  btnSearch->setFixedWidth(150);

  leftLayout->addWidget(buttonContainer);

  // Правая часть: чекбоксы и лейблы с количеством
  rightWidget = new QWidget(centralWidget);
  rightWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  rightWidget->setFixedWidth(280);

  QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
  rightLayout->setContentsMargins(15, 15, 15, 15);
  rightLayout->setSpacing(12);
  rightLayout->setAlignment(Qt::AlignTop);

  // Создаем чекбоксы с полными названиями
  QStringList partNames = {"Подлежащее",  "Сказуемое",      "Дополнение",
                           "Определение", "Обстоятельство", "Другое"};

  for (int i = 0; i < partNames.size(); ++i) {
    QWidget *rowWidget = new QWidget(rightWidget);
    QHBoxLayout *hLayout = new QHBoxLayout(rowWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(8);

    QCheckBox *cb = new QCheckBox(partNames[i], rowWidget);
    cb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cb->setFixedWidth(110);

    m_checkBoxes.append(cb);

    countLabels[i] = new QLabel("0", rowWidget);
    countLabels[i]->setMinimumWidth(35);
    countLabels[i]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    countLabels[i]->setStyleSheet("font-weight: bold;");

    bars[i] = new QProgressBar(rowWidget);
    bars[i]->setTextVisible(false);
    bars[i]->setRange(0, 1);
    bars[i]->setFixedWidth(90);
    bars[i]->setFixedHeight(20);

    hLayout->addWidget(cb);
    hLayout->addWidget(countLabels[i]);
    hLayout->addWidget(bars[i]);
    hLayout->addStretch();

    rightLayout->addWidget(rowWidget);
  }

  rightLayout->addStretch();

  mainLayout->addWidget(leftWidget, 1);
  mainLayout->addWidget(rightWidget, 0);

  connect(btnSearch, &QPushButton::clicked, this, &ResultPage::searchRequested);

  fullText = "";
}

/**
 * @brief Считывает текст из файла во внутреннюю переменную.
 * @param filename Путь к файлу с текстом.
 */
void ResultPage::readTextFromFile(const QString &filename) {
  QFile file(filename);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);
  fullText = in.readAll();
  file.close();
}

/**
 * @brief Загружает текст из файла для отображения.
 * @param filename Путь к файлу с текстом.
 */
void ResultPage::loadTextFromFile(const QString &filename) {
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    fullText = in.readAll();
    file.close();
  } else {
    fullText = "Не удалось открыть файл с текстом.";
  }

  if (widgetText) {
    widgetText->setMarkupText(fullText, members);
  }
}

/**
 * @brief Обновляет отображение количества элементов в каждой категории.
 */
void ResultPage::updateCounts() {
  countLabels[0]->setText(QString::number(parts.subject.size()));
  countLabels[1]->setText(QString::number(parts.predicate.size()));
  countLabels[2]->setText(QString::number(parts.object.size()));
  countLabels[3]->setText(QString::number(parts.attribute.size()));
  countLabels[4]->setText(QString::number(parts.adverbial.size()));
  countLabels[5]->setText(QString::number(parts.other.size()));
}

/**
 * @brief Загружает разобранные данные из файла.
 * @param filename Путь к файлу с данными разбора.
 */
void ResultPage::loadParsedData(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  QTextStream in(&file);
  in.setEncoding(QStringConverter::Utf8);

  parts.subject.clear();
  parts.predicate.clear();
  parts.object.clear();
  parts.attribute.clear();
  parts.adverbial.clear();
  parts.other.clear();

  while (!in.atEnd()) {
    QString line = in.readLine().trimmed();
    if (line.isEmpty())
      continue;

    QStringList keyValue = line.split(": ", Qt::SkipEmptyParts);
    if (keyValue.size() != 2)
      continue;

    QString category = keyValue[0].trimmed();
    QString wordsString = keyValue[1];
    QStringList wordsList = wordsString.split(',', Qt::SkipEmptyParts);

    if (category == "Подлежащее") {
      for (const QString &word : wordsList) {
        parts.subject.append(word.trimmed());
      }
    } else if (category == "Сказуемое") {
      for (const QString &word : wordsList) {
        parts.predicate.append(word.trimmed());
      }
    } else if (category == "Дополнение") {
      for (const QString &word : wordsList) {
        parts.object.append(word.trimmed());
      }
    } else if (category == "Определение") {
      for (const QString &word : wordsList) {
        parts.attribute.append(word.trimmed());
      }
    } else if (category == "Обстоятельство") {
      for (const QString &word : wordsList) {
        parts.adverbial.append(word.trimmed());
      }
    } else if (category == "Другое") {
      for (const QString &word : wordsList) {
        parts.other.append(word.trimmed());
      }
    }
  }
  file.close();
}

/**
 * @brief Обновляет графическое отображение (прогресс-бары) статистики.
 */
void ResultPage::updateChart() {
  int counts[] = {static_cast<int>(parts.subject.size()),
                  static_cast<int>(parts.predicate.size()),
                  static_cast<int>(parts.object.size()),
                  static_cast<int>(parts.attribute.size()),
                  static_cast<int>(parts.adverbial.size()),
                  static_cast<int>(parts.other.size())};

  int total = 0;
  for (int count : counts) {
    total += count;
  }

  if (total == 0) {
    for (int i = 0; i < 6; ++i) {
      bars[i]->setValue(0);
      bars[i]->setRange(0, 1);
    }
    return;
  }

  for (int i = 0; i < 6; ++i) {
    bars[i]->setRange(0, total);
    bars[i]->setValue(counts[i]);
  }
}

/**
 * @brief Слот, обрабатывающий нажатие кнопки поиска.
 */
void ResultPage::onSearchClicked() { emit searchRequested(); }

/**
 * @brief Строит карту соответствия слов их ролям в предложении.
 */
void ResultPage::buildWordRoleMap() {
  members.clear();

  if (fullText.isEmpty()) {
    if (widgetText) {
      widgetText->setMarkupText("", members);
    }
    return;
  }

  QRegularExpression re("\\p{L}+");
  QRegularExpressionMatchIterator i = re.globalMatch(fullText);

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString word = match.captured(0);

    if (parts.subject.contains(word)) {
      members.insert(word, "подл.");
    } else if (parts.predicate.contains(word)) {
      members.insert(word, "сказ.");
    } else if (parts.object.contains(word)) {
      members.insert(word, "доп.");
    } else if (parts.attribute.contains(word)) {
      members.insert(word, "опр.");
    } else if (parts.adverbial.contains(word)) {
      members.insert(word, "обст.");
    } else {
      members.insert(word, "др.");
    }
  }

  if (widgetText) {
    widgetText->setMarkupText(fullText, members);
  }
}

/**
 * @brief Обновляет все отображения на интерфейсе.
 */
void ResultPage::refreshDisplay() {
  updateCounts();
  updateChart();
  if (widgetText) {
    widgetText->setMarkupText(fullText, members);
  }
}
