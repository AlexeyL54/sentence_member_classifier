//#include "mainwindow.h"
#include "ResultPage.hpp"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QVector>
#include <QString>
#include <QRegularExpression>
#include <QStyle>

ResultPage::ResultPage(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Анализ предложения");
    resize(1200, 400);

    readTextFromFile("text.txt");
    loadParsedData("data.txt");
    buildWordRoleMap();

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Левая часть: текстовое поле (только для чтения)
    leftWidget = new QWidget(centralWidget);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(10, 10, 10, 10);
    leftLayout->setSpacing(10);

    widgetText = new TextMarkupWidget(centralWidget);
    widgetText->setMarkupText(fullText, members);
    // Настраиваем цвета
    widgetText->setWordColor("#000000");  // тёмно-синий для слов
    widgetText->setLabelColor("#000000"); // серый для подписей
    widgetText->setMinimumSize(800, 400);
    leftLayout->addWidget(widgetText);

/*    textEdit = new QTextEdit(leftWidget);
    textEdit->setMinimumSize(600,400);
    textEdit->setReadOnly(true); // Запрещаем редактирование
    textEdit->setPlaceholderText("Текст будет загружен из файла...");
    leftLayout->addWidget(textEdit);

    loadTextFromFile("text.txt"); */

    //mainLayout->addWidget(textEdit, 2);

    // 2. Нижняя часть: Кнопки
        QWidget *buttonContainer = new QWidget(leftWidget);
        QHBoxLayout *buttonLayout = new QHBoxLayout(buttonContainer);

        // Кнопка Сохранить
        btnSave = new QPushButton("Сохранить", buttonContainer);
        // Кнопка Поиск
        btnSearch = new QPushButton("Поиск", buttonContainer);

        // Добавляем кнопки в горизонтальный макет
        buttonLayout->addWidget(btnSave);
        buttonLayout->addWidget(btnSearch);

        btnSave->setFixedWidth(200);
        btnSearch->setFixedWidth(200);


        // Добавляем "растяжку" (spacer), чтобы прижать кнопки к левому краю или центру
        buttonLayout->addStretch();

        leftLayout->addWidget(buttonContainer);
        // Добавляем контейнер с кнопками в главный вертикальный макет (вниз)
        mainLayout->addWidget(leftWidget);


    /*
    // Правая часть: чекбоксы с членами предложения
    rightWidget = new QWidget(centralWidget);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(10, 10, 10, 10);
    rightLayout->setSpacing(10);

    QStringList parts = {"Подлежащее", "Сказуемое", "Дополнение", "Определение", "Обстоятельство", "Другое"};
    for (const QString &part : parts) {
        QCheckBox *cb = new QCheckBox(part, rightWidget);
        rightLayout->addWidget(cb);
    }
    */

    // Правая часть: чекбоксы и лейблы с количеством
        rightWidget = new QWidget(centralWidget);
        QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
        rightLayout->setContentsMargins(10, 10, 10, 10);
        rightLayout->setSpacing(10);

        QStringList parts = {"Подлежащее", "Сказуемое", "Дополнение", "Определение", "Обстоятельство", "Другое"};
        for (int i = 0; i < parts.size(); ++i) {
            QHBoxLayout *hLayout = new QHBoxLayout();
            QCheckBox *cb = new QCheckBox(parts[i], rightWidget);
            countLabels[i] = new QLabel("0", rightWidget); // Лейбл для количества
            countLabels[i]->setMinimumWidth(30);
            hLayout->addWidget(cb);
            hLayout->addWidget(countLabels[i]);
            bars[i] = new QProgressBar(rightWidget);
            bars[i]->setTextVisible(false); // Скрываем проценты
            bars[i]->setRange(0, 1); // Диапазон будет установлен позже
            bars[i]->setFixedWidth(120); // Ширина столбца диаграммы
            //hLayout->addWidget(cb);
            hLayout->addWidget(bars[i]);
            rightLayout->addLayout(hLayout);
        }

       // Добавляем текстовое поле для подробностей под чекбоксами
     //  detailsTextEdit = new QTextEdit(rightWidget);
       // detailsTextEdit->setReadOnly(true); // Только для чтения
        //detailsTextEdit->setPlaceholderText("");
       // rightLayout->addWidget(detailsTextEdit);

      //  rightLayout->addStretch();

        //loadParsedData("data.txt");
        updateCounts(); // Обновляем отображение количества
        updateChart();

    // Прокрутка для правой части
    QScrollArea *scrollArea = new QScrollArea(centralWidget);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(rightWidget);
    mainLayout->addWidget(scrollArea, 1);



}

// Функция принимает имя файла и возвращает текст в виде QString
void ResultPage::readTextFromFile(const QString &filename)
{
    QFile file(filename);

    // 1. Пытаемся открыть файл
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Если открыть не удалось, выводим отладочное сообщение
        //qDebug() << "Ошибка открытия файла:" << filename;
        return; // Возвращаем пустую строку
    }

    // 2. Создаем поток для чтения из файла
    QTextStream in(&file);

    // 3. Устанавливаем кодировку (критически важно для русского языка)
    in.setCodec("UTF-8");

    // 4. Считываем ВЕСЬ текст из файла в переменную
    fullText = in.readAll();

    // 5. Закрываем файл
    file.close();

}

void ResultPage::loadTextFromFile(const QString &filename)
{

    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setCodec("UTF-8");
        textEdit->setPlainText(in.readAll());
        fullText = in.readAll();
    file.close();
    }
    else {
        textEdit->setPlainText("Не удалось открыть файл с текстом.");
    }
}

// Функция обновления количества рядом с чекбоксами
void ResultPage::updateCounts()
{
    countLabels[0]->setText(QString::number(parts.subject.size()));
    countLabels[1]->setText(QString::number(parts.predicate.size()));
    countLabels[2]->setText(QString::number(parts.object.size()));
    countLabels[3]->setText(QString::number(parts.attribute.size()));
    countLabels[4]->setText(QString::number(parts.adverbial.size()));
    // Для "Другое" можно оставить 0 или реализовать по необходимости
}

// --- НОВАЯ ФУНКЦИЯ ЗАГРУЗКИ В СТРУКТУРУ ---
void ResultPage::loadParsedData(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
       // textEdit->setPlainText("Не удалось открыть файл с текстом.");
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    // Очищаем все векторы перед загрузкой новых данных
    parts.subject.clear();
    parts.predicate.clear();
    parts.object.clear();
    parts.attribute.clear();
    parts.adverbial.clear();
    parts.other.clear();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList keyValue = line.split(": ", Qt::SkipEmptyParts);
        if (keyValue.size() != 2) continue;

        QString category = keyValue[0].trimmed();
        QString wordsString = keyValue[1];

        // Разбиваем строку слов на список и чистим пробелы
        QStringList wordsList = wordsString.split(',', Qt::SkipEmptyParts);

        // Заполняем соответствующий вектор в структуре
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
        }
        else if (category == "Другое") {
             for (const QString &word : wordsList) {
                        parts.other.append(word.trimmed());
             }
        }
    }
    file.close();
   // updateCounts(); // Обновляем отображение количества
   // updateChart(); // Обновляем диаграмму
}
// --- КОНЕЦ ФУНКЦИИ ---

// Функция обновления количества рядом с чекбоксами

// Функция обновления диаграммы
void ResultPage::updateChart()
{
    // Получаем размеры векторов
    int counts[] = {
        parts.subject.size(),
        parts.predicate.size(),
        parts.object.size(),
        parts.attribute.size(),
        parts.adverbial.size(),
        parts.other.size() // Для "Другое"
    };

    // Вычисляем сумму всех членов предложения (максимум шкалы)
        int total = 0;
        for (int count : counts) {
            total += count;
        }

    // Если данных нет, скрываем или обнуляем диаграмму
    if (total == 0) {
        for (int i = 0; i < 6; ++i) {
            bars[i]->setValue(0);
            bars[i]->setRange(0, 1);
        }
        return;
    }

    // Устанавливаем одинаковый максимум для всех и обновляем значения
        for (int i = 0; i < 6; ++i) {
            bars[i]->setRange(0, total);
            bars[i]->setValue(counts[i]);
        }
}

/*
void MainWindow::buildWordRoleMap()
{
    // 1. Получаем текст из textEdit и очищаем его от лишних символов
    //QString fullText = textEdit->toPlainText();

    // Используем регулярное выражение для разбиения на слова.
    // \w+ находит последовательности из букв, цифр и знака нижнего подчёркивания.
    // Это лучше, чем split(' '), так как убирает знаки препинания.
    QRegularExpression re("\\w+");
    QRegularExpressionMatchIterator i = re.globalMatch(fullText);


    // 3. Проходим по всем найденным словам
    while (i.hasNext()) {
        QRegularExpressionMatch match = t.next();
        QString word = match.captured(0); // Получаем текущее слово

        int pos_sbj = 0;
        int pos_pr = 0;
        int pos_obj = 0;
        int pos_atr = 0;
        int pos_adv = 0;


        if (parts.subject.at(pos_sbj) == word) {
            members.insert(word, "подл.");
            pos_sbj++;
        }
        else if (parts.predicate.at(pos_pr) == word) {
            members.insert(word, "сказ.");
            pos_pr++;
        }
        else if (parts.object.at(pos_obj) == word) {
            members.insert(word, "доп.");
            pos_obj++;
        }
        else if (parts.attribute.at(pos_atr) == word) {
            members.insert(word, "опр.");
            pos_atr++;
        }
        else if (parts.adverbial.at(pos_adv) == word) {
            members.insert(word, "обст.");
            pos_adv++;
        }
        else {
            members.insert(word, "др.");
        }
    }
}
*/

void ResultPage::buildWordRoleMap()
{
    // 1. Очищаем карту перед новой обработкой
    members.clear();

    // 2. Используем регулярное выражение для разбиения на слова.
    //    \w+ находит последовательности из букв, цифр и знака нижнего подчёркивания.
    //    Это лучше, чем split(' '), так как автоматически убирает знаки препинания.
    QRegularExpression re("\\p{L}+");
    QRegularExpressionMatchIterator i = re.globalMatch(fullText);

    // 3. Проходим по всем найденным словам
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(0); // Получаем текущее слово

        // 4. Проверяем принадлежность слова к категориям БЕЗ СЧЕТЧИКОВ.
        //    Метод .contains() просто ищет слово в списке.
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
            members.insert(word, "др."); // Если слово не найдено ни в одном списке
        }
    }
}





// -------------------- Реализация FlowLayout --------------------

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) {
  setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing) {
  setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout() {
  QLayoutItem *item;
  while ((item = takeAt(0)))
    delete item;
}

void FlowLayout::addItem(QLayoutItem *item) { m_itemList.append(item); }

int FlowLayout::horizontalSpacing() const {
  if (m_hSpace >= 0)
    return m_hSpace;
  return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const {
  if (m_vSpace >= 0)
    return m_vSpace;
  return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

Qt::Orientations FlowLayout::expandingDirections() const {
  return Qt::Horizontal;
}

bool FlowLayout::hasHeightForWidth() const { return true; }

int FlowLayout::heightForWidth(int width) const {
  return doLayout(QRect(0, 0, width, 0), true);
}

int FlowLayout::count() const { return m_itemList.size(); }

QLayoutItem *FlowLayout::itemAt(int index) const {
  return m_itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index) {
  if (index >= 0 && index < m_itemList.size())
    return m_itemList.takeAt(index);
  return nullptr;
}

void FlowLayout::setGeometry(const QRect &rect) {
  QLayout::setGeometry(rect);
  doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const { return minimumSize(); }

QSize FlowLayout::minimumSize() const {
  QSize size;
  for (const QLayoutItem *item : m_itemList)
    size = size.expandedTo(item->minimumSize());

  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  size += QSize(left + right, top + bottom);
  return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();
  int y = effectiveRect.y();
  int lineHeight = 0;

  for (QLayoutItem *item : m_itemList) {
    QWidget *wid = item->widget();
    int spaceX = horizontalSpacing();
    if (spaceX == -1)
      spaceX = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    int spaceY = verticalSpacing();
    if (spaceY == -1)
      spaceY = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

    int nextX = x + item->sizeHint().width() + spaceX;
    if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
      x = effectiveRect.x();
      y = y + lineHeight + spaceY;
      nextX = x + item->sizeHint().width() + spaceX;
      lineHeight = 0;
    }

    if (!testOnly)
      item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

    x = nextX;
    lineHeight = qMax(lineHeight, item->sizeHint().height());
  }
  return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const {
  QObject *parent = this->parent();
  if (!parent) {
    return -1;
  } else if (parent->isWidgetType()) {
    QWidget *pw = static_cast<QWidget *>(parent);
    return pw->style()->pixelMetric(pm, nullptr, pw);
  } else {
    return static_cast<QLayout *>(parent)->spacing();
  }
}

// -------------------- Реализация TextMarkupWidget --------------------

TextMarkupWidget::TextMarkupWidget(QWidget *parent)
    : QWidget(parent), m_wordColor("#000000"), m_labelColor("#555555") {
  m_container = new QWidget();
  m_flowLayout = new FlowLayout(m_container, 5, 5, 5);
  m_container->setLayout(m_flowLayout);

  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidget(m_container);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_scrollArea->setStyleSheet(
      "QScrollArea { border: none; background-color: transparent; }"
      "QScrollArea > QWidget > QWidget { background-color: transparent; }");

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(m_scrollArea);
}

void TextMarkupWidget::setMarkupText(const QString &text, const QMap<QString, QString> &members) {
  m_text = text;
  m_members = members;
  rebuild();
}

void TextMarkupWidget::setWordColor(const QString &color) {
  if (m_wordColor != color) {
    m_wordColor = color;
    rebuild();
  }
}

void TextMarkupWidget::setLabelColor(const QString &color) {
  if (m_labelColor != color) {
    m_labelColor = color;
    rebuild();
  }
}

QString TextMarkupWidget::wordColor() const { return m_wordColor; }

QString TextMarkupWidget::labelColor() const { return m_labelColor; }

void TextMarkupWidget::rebuild() {
  // Очищаем layout
  QLayoutItem *child;
  while ((child = m_flowLayout->takeAt(0)) != nullptr) {
    delete child->widget();
    delete child;
  }

  QStringList words = m_text.split(" ", Qt::SkipEmptyParts);

  for (const QString &word : words) {
    // Создаем вертикальный layout для слова и подписи
    QVBoxLayout *wordLayout = new QVBoxLayout();
    wordLayout->setSpacing(0);
    wordLayout->setContentsMargins(2, 0, 2, 0);

    // Подпись члена предложения
    QLabel *memberLabel = new QLabel(m_members.value(word, "-"));
    memberLabel->setAlignment(Qt::AlignCenter);
    memberLabel->setStyleSheet(
        QString("font-size: 7pt; color: %1; font-weight: normal; "
                "letter-spacing: 0.5px;")
            .arg(m_labelColor));

    // Само слово
    QLabel *wordLabel = new QLabel(word);
    wordLabel->setAlignment(Qt::AlignCenter);
    wordLabel->setStyleSheet(
        QString("font-size: 11pt; color: %1; font-weight: normal;")
            .arg(m_wordColor));

    wordLayout->addWidget(memberLabel);
    wordLayout->addWidget(wordLabel);

    // Контейнер для wordLayout
    QWidget *container = new QWidget();
    container->setLayout(wordLayout);

    // Добавляем в flow layout
    m_flowLayout->addWidget(container);
  }
}
