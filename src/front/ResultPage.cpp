#include "ResultPage.hpp"
#include "TextMarkupWidget.hpp"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>
#include <QVector>
#include <QSizePolicy>

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
  btnAnalize = new QPushButton("Новый Анализ", buttonContainer);

  buttonLayout->addWidget(btnSave);
  buttonLayout->addWidget(btnSearch);
  buttonLayout->addWidget(btnAnalize);
  buttonLayout->addStretch();

  btnSave->setFixedWidth(150);
  btnSearch->setFixedWidth(150);
  btnAnalize->setFixedWidth(150);

  leftLayout->addWidget(buttonContainer);

  // Правая часть: чекбоксы и лейблы с количеством
  rightWidget = new QWidget(centralWidget);
  rightWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  rightWidget->setFixedWidth(300);

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

    connect(cb, &QCheckBox::stateChanged, this, &ResultPage::onCheckboxStateChanged);

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


  // --- ДОБАВЛЕНИЕ СТАТИСТИКИ ---

  statsWidget = new QWidget(rightWidget);
  QVBoxLayout *statsLayout = new QVBoxLayout(statsWidget);
  statsLayout->setContentsMargins(15, 25, 15, 15); // Отступ сверху
  statsLayout->setSpacing(6);
  statsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

  QLabel *statsTitle = new QLabel("Статистика", statsWidget);
  statsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
  statsLayout->addWidget(statsTitle);

  // Создаем лейблы и сохраняем их в поля класса для будущего доступа
  labelSentences = new QLabel("Предложений: 0", statsWidget);
  labelWords = new QLabel("Слов: 0", statsWidget);
  labelMembers = new QLabel("Членов предложения: 0", statsWidget);
  labelSubjects = new QLabel("Подлежащих: 0", statsWidget);
  labelPredicates = new QLabel("Сказуемых: 0", statsWidget);
  labelDefinitions = new QLabel("Определений: 0", statsWidget);
  labelAdditions = new QLabel("Дополнений: 0", statsWidget);
  labelAdverbials = new QLabel("Обстоятельств: 0", statsWidget);
  top_subject = new QLabel("самое популярное подлежащее: ", statsWidget);
  top_predicate = new QLabel("самое популярное сказуемое: ", statsWidget);
  top_definition = new QLabel("самое популярное определение: ", statsWidget);
  top_addition = new QLabel("самое популярное дополнение: ", statsWidget);
  top_adverbial = new QLabel("самое популярное обстоятельство: ", statsWidget);

  // Добавляем лейблы в layout виджета
  statsLayout->addWidget(labelSentences);
  statsLayout->addWidget(labelWords);
  statsLayout->addWidget(labelMembers);
  statsLayout->addWidget(labelSubjects);
  statsLayout->addWidget(labelPredicates);
  statsLayout->addWidget(labelDefinitions);
  statsLayout->addWidget(labelAdditions);
  statsLayout->addWidget(labelAdverbials);
  statsLayout->addWidget(top_subject);
  statsLayout->addWidget(top_predicate);
  statsLayout->addWidget(top_definition);
  statsLayout->addWidget(top_addition);
  statsLayout->addWidget(top_adverbial);

  // Добавляем растяжку, чтобы статистика была вверху
  statsLayout->addStretch();

  // Добавляем готовый виджет в основной правый layout
  rightLayout->addWidget(statsWidget);

  rightLayout->addStretch();

  mainLayout->addWidget(leftWidget, 1);
  mainLayout->addWidget(rightWidget, 0);

  connect(btnSearch, &QPushButton::clicked, this, &ResultPage::searchRequested);
  connect(btnAnalize, &QPushButton::clicked, this, &ResultPage::onAnalyzeClicked);

  fullText = "";

  updateStatsDisplay();
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
  in.setCodec("UTF-8");
  fullText = in.readAll();
  file.close();
}

/**
 * @brief Загружает текст из файла для отображения.
 * @param filename Путь к файлу с текстом.
 */
/*void ResultPage::loadTextFromFile(const QString &filename) {
  QFile file(filename);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    in.setCodec("UTF-8");  //setEncoding(QStringConverter::Utf8);
    fullText = in.readAll();
    file.close();
  } else {
    fullText = "Не удалось открыть файл с текстом.";
  }

  if (widgetText) {
    widgetText->setMarkupText(fullText, members);
  }
}*/

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
/*void ResultPage::loadParsedData(const QString &filename) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  QTextStream in(&file);
  in.setCodec("UTF-8"); //in.setEncoding(QStringConverter::Utf8);

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
}*/

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
/*void ResultPage::buildWordRoleMap() {
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
}*/

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

void ResultPage::onAnalyzeClicked() {
  QString str = "";
  if(isSaved)
  {
     str = "Вы уверены, что хотите начать новый анализ?";
  }
  else
  {
     str = "Новый анализ приведет к потере несохранненных данных.\nНачать новый анализ?";
  }

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Анализ");
  msgBox.setText(str);
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//  msgBox.setDefaultButton(QMessageBox::Yes);

  int ret = msgBox.exec();

  if (ret == QMessageBox::Yes) {
    // Пользователь выбрал "Да" — здесь можно вызвать логику анализа
    QMessageBox::information(this, "Анализ", "Анализ выполнен!");
  } else {
    // Пользователь выбрал "Нет" или закрыл окно
    QMessageBox::information(this, "Отмена", "Анализ отменён.");
  }
}

void ResultPage::onCheckboxStateChanged(int state)
{
    // Получаем указатель на чекбокс, который отправил сигнал
    QObject *senderObj = sender();
    QCheckBox *currentCheckbox = qobject_cast<QCheckBox*>(senderObj);

    // Проверяем, что объект найден
    if (!currentCheckbox) {
        return;
    }

    // Если галочку установили
    if (state == Qt::Checked) {
        // Блокируем все остальные
        foreach (QCheckBox *box, m_checkBoxes) {
            if (box != currentCheckbox) {
                box->setEnabled(false);
                // Галочку с других снимаем на всякий случай
                box->setChecked(false);
            }
        }
    }
    // Если галочку сняли (отжали)
    else if (state == Qt::Unchecked) {
        // Разблокируем все чекбоксы в списке
        foreach (QCheckBox *box, m_checkBoxes) {
            box->setEnabled(true);
        }
    }
}



void ResultPage::setData(const std::vector<SentenceResult>& results)
{
    m_results = results;

    // 1. Очищаем старые данные о членах предложения
    parts.subject.clear();
    parts.predicate.clear();
    parts.object.clear();
    parts.attribute.clear();
    parts.adverbial.clear();
    parts.other.clear();

    if (m_results.empty()) {
        fullText = "Нет данных для отображения.";
        members.clear();
        refreshDisplay();
        return;
    }

    // Формируем fullText из всех предложений
    QStringList allSentences;
    for (const auto& sentence : m_results) {
        allSentences.append(QString::fromStdString(sentence.text));
    }
    // Объединяем предложения в один текст с переводом строки
    fullText = allSentences.join("\n\n");

    // 2. Проходим по каждому предложению для сбора статистики (parts)
    for (const auto& sentence : m_results) {
        if (sentence.entities.size() != sentence.tokens.size()) {
            continue;
        }

        for (size_t i = 0; i < sentence.tokens.size(); ++i) {
            const std::string& token = sentence.tokens[i];
            const std::string& word = sentence.entities[i];
            QString qWord = QString::fromStdString(word);

            if (token == "подл.") {
                parts.subject.append(qWord);
            } else if (token == "сказ.") {
                parts.predicate.append(qWord);
            } else if (token == "доп.") {
                parts.object.append(qWord);
            } else if (token == "опр.") {
                parts.attribute.append(qWord);
            } else if (token == "обст.") {
                parts.adverbial.append(qWord);
            } else {
                parts.other.append(qWord);
            }
        }
    }

    // 3. Строим карту разметки для виджета (берем только первое предложение)
    buildWordRoleMap();

}

void ResultPage::buildWordRoleMap() {
    members.clear();

    if (m_results.empty() || fullText.isEmpty()) {
        if (widgetText) {
            widgetText->setMarkupText(fullText, members);
        }
        return;
    }

    for (const auto& sentence : m_results){
    //const SentenceResult& sentence = m_results[0]; // Берем первое предложение

    // Проходим по всем токенам и словам из структуры данных
    for (size_t i = 0; i < sentence.entities.size(); ++i) {
        QString word = QString::fromStdString(sentence.entities[i]);
        QString role = QString::fromStdString(sentence.tokens[i]);

        members.insert(word, role);
    }
    }

    if (widgetText) {
        widgetText->setMarkupText(fullText, members);
    }
}

std::vector<SentenceResult> ResultPage::makeData() {
    std::vector<SentenceResult> data;

    SentenceResult result;

    result.text = "На их пути расцветали весенние цветы, зеленела трава.";
    result.entities = {"На", "их", "пути", "расцветали", "весенние", "цветы", "зеленела", "трава"};
    result.tokens = {"обст.", "обст.", "обст.", "сказ.", "опр.", "подл.", "сказ.", "подл."};
    data.push_back(result);

    result.text = "Вот раздался колокольный звон.";
    result.entities = {"Вот", "раздался", "колокольный", "звон"};
    result.tokens = {"обст.", "сказ.", "опр.", "подл."};
    data.push_back(result);

    result.text = "Кай и Герда узнали колокольни родного города.";
    result.entities = {"Кай", "и", "Герда", "узнали", "колокольни", "родного", "города"};
    result.tokens = {"подл.", "др.", "подл.", "сказ.", "доп.", "опр.", "доп."};
    data.push_back(result);

    result.text = "Они поднялись по знакомой лестнице и вошли в комнату.";
    result.entities = {"Они", "поднялись", "по", "знакомой", "лестнице", "и", "вошли", "в", "комнату"};
    result.tokens = {"подл.", "сказ.", "др.", "опр.", "обст.", "др.", "сказ.", "др.", "обст."};
    data.push_back(result);

    result.text = "Здесь ничего не изменилось.";
    result.entities = {"Здесь", "ничего", "не", "изменилось"};
    result.tokens = {"обст.", "доп.", "др.", "сказ."};
    data.push_back(result);

    result.text = "Цветущие розовые кусты заглядывали с крыши в открытое окошко.";
    result.entities = {"Цветущие", "розовые", "кусты", "заглядывали", "с", "крыши", "в", "открытое", "окошко"};
    result.tokens = {"опр.", "опр.", "подл.", "сказ.", "др.", "обст.", "др.", "опр.", "доп."};
    data.push_back(result);

    result.text = "Тут же стояли их детские стульчики.";
    result.entities = {"Тут", "же", "стояли", "их", "детские", "стульчики"};
    result.tokens = {"обст.", "обст.", "сказ.", "опр.", "опр.", "подл."};
    data.push_back(result);

    result.text = "Кай с Гердой сели каждый на свой и взялись за руки.";
    result.entities = {"Кай", "с", "Гердой", "сели", "каждый", "на", "свой", "и", "взялись", "за", "руки"};
    result.tokens = {"подл.", "др.", "доп.", "сказ.", "опр.", "др.", "обст.", "др.", "сказ.", "др.", "доп."};
    data.push_back(result);

    result.text = "Холодное великолепие чертогов Снежной королевы забылось.";
    result.entities = {"Холодное", "великолепие", "чертогов", "Снежной", "королевы", "забылось"};
    result.tokens = {"опр.", "подл.", "доп.", "опр.", "опр.", "сказ."};
    data.push_back(result);

    result.text = "Они стали совсем взрослыми, но были детьми сердцем и душой.";
    result.entities = {"Они", "стали", "совсем", "взрослыми", "но", "были", "детьми", "сердцем", "и", "душой"};
    result.tokens = {"подл.", "сказ.", "обст.", "опр.", "др.", "сказ.", "доп.", "обст.", "др.", "обст."};
    data.push_back(result);

    result.text = "На дворе стояло тёплое благодатное лето.";
    result.entities = {"На", "дворе", "стояло", "тёплое", "благодатное", "лето"};
    result.tokens = {"обст.", "обст.", "сказ.", "опр.", "опр.", "доп."};
    data.push_back(result);

    return data;
}


/**
 * @brief Обновляет отображение статистики на интерфейсе.
 * @param stats Структура с готовыми данными для отображения.
 */
void ResultPage::updateStatsDisplay(/*GlobalStats& stats*/) {
    // Проверяем, что виджет был создан (чтобы избежать краша при старте)
    if (!statsWidget) return;

    // Обновляем текст каждого лейбла, используя данные из структуры
    labelSentences->setText(QString("Предложений: %1").arg(stats.sentences_total));
    labelWords->setText(QString("Слов: %1").arg(stats.words_total));
    labelMembers->setText(QString("Членов предложения: %1").arg(stats.members_total));

    labelSubjects->setText(QString("Подлежащих: %1").arg(stats.subjects_total));
    labelPredicates->setText(QString("Сказуемых: %1").arg(stats.predicates_total));

    labelDefinitions->setText(QString("Определений: %1").arg(stats.definitions_total));
    labelAdditions->setText(QString("Дополнений: %1").arg(stats.additions_total));
    labelAdverbials->setText(QString("Обстоятельств: %1").arg(stats.adverbials_total));

    // Обновляем топ-слова (самые популярные)
    // Используем QString::fromStdString для перевода std::string -> QString


    top_subject->setText(QString("самое популярное подлежащее: %1").arg(QString::fromStdString(stats.top_subject.first)));
    top_predicate->setText(QString("самое популярное сказуемое: %1").arg(QString::fromStdString(stats.top_predicate.first)));
    top_definition->setText(QString("самое популярное определение: %1").arg(QString::fromStdString(stats.top_definition.first)));
    top_addition->setText(QString("самое популярное дополнение: %1").arg(QString::fromStdString(stats.top_addition.first)));
    top_adverbial->setText(QString("самое популярное обстоятельство: %1").arg(QString::fromStdString(stats.top_adverbial.first)));
}
