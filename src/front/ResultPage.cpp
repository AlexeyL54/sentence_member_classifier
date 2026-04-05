#include "ResultPage.hpp"
#include "lib/TextMarkupWidget.hpp"
#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSizePolicy>
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
  QStringList roles = {"подл.", "сказ.", "доп.", "опр.", "обст.", "др."};

  for (int i = 0; i < partNames.size(); ++i) {
    QWidget *rowWidget = new QWidget(rightWidget);
    QHBoxLayout *hLayout = new QHBoxLayout(rowWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(8);

    QCheckBox *cb = new QCheckBox(partNames[i], rowWidget);
    cb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cb->setFixedWidth(110);

    QString role = roles.at(i);
    // connect(cb, &QCheckBox::stateChanged, this,
    // &ResultPage::onCheckboxStateChanged);
    connect(cb, &QCheckBox::checkStateChanged, this,
            [this, role](int state) { onCheckboxStateChanged(state, role); });

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
  connect(btnAnalize, &QPushButton::clicked, this,
          &ResultPage::onAnalyzeClicked);

  fullText = "";

  updateStatsDisplay();
}

/**
 * @brief Считывает текст из файла во внутреннюю переменную.
 * @param filename Путь к файлу с текстом.
 */
/*void ResultPage::readTextFromFile(const QString &filename) {
  QFile file(filename);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return;
  }

  QTextStream in(&file);
  in.setCodec("UTF-8");
  fullText = in.readAll();
  file.close();
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
  if (isSaved) {
    str = "Вы уверены, что хотите начать новый анализ?";
  } else {
    str = "Новый анализ приведет к потере несохранненных данных.\nНачать новый "
          "анализ?";
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

void ResultPage::onCheckboxStateChanged(int state, const QString &role) {
  // Получаем указатель на чекбокс, который отправил сигнал
  QObject *senderObj = sender();
  QCheckBox *currentCheckbox = qobject_cast<QCheckBox *>(senderObj);

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

    // Вызываем функцию выделения текста
    if (widgetText) {
      widgetText->setHighlightedRole(role);
    }
  }
  // Если галочку сняли (отжали)
  else if (state == Qt::Unchecked) {
    // Разблокируем все чекбоксы в списке
    foreach (QCheckBox *box, m_checkBoxes) {
      box->setEnabled(true);
    }

    // Если галочка снята со всех (или мы просто хотим снять выделение),
    // передаем пустую строку, чтобы убрать фон.
    if (widgetText) {
      widgetText->setHighlightedRole("");
    }
  }
}

void ResultPage::setData(const std::vector<SentenceResult> &results) {
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
  for (const SentenceResult &sentence : m_results) {
    allSentences.append(QString::fromStdString(sentence.text));
  }
  // Объединяем предложения в один текст с переводом строки
  fullText = allSentences.join("\n\n");

  // 2. Проходим по каждому предложению для сбора статистики (parts)
  for (const SentenceResult &sentence : m_results) {
    /*if (sentence.entities.size() != sentence.tokens.size()) {
      continue;
    }*/

    for (size_t i = 0; i < sentence.tokens.size(); ++i) {
      const std::string &type = sentence.entities[i].type_ru;
      const std::string &sentence_part = sentence.entities[i].text;
      QString qWord = QString::fromStdString(sentence_part);

      if (type == "подлeжащее") {
        parts.subject.append(qWord);
      } else if (type == "сказуемое") {
        parts.predicate.append(qWord);
      } else if (type == "дополнение") {
        parts.object.append(qWord);
      } else if (type == "определение") {
        parts.attribute.append(qWord);
      } else if (type == "обстоятельство") {
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

  for (const SentenceResult &sentence : m_results) {
    // const SentenceResult& sentence = m_results[0]; // Берем первое
    // предложение

    // Проходим по всем видам членов предложения и словам из структуры данных
    for (size_t i = 0; i < sentence.entities.size(); ++i) {
      QString word = QString::fromStdString(sentence.entities[i].text);
      QString role = QString::fromStdString(sentence.entities[i].type_ru);

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

  // 1
  result.text = "На их пути расцветали весенние цветы, зеленела трава.";
  result.entities = {{"На", "B-OBST", "обстоятельство", 0, 2},
                     {"их", "B-OBST", "обстоятельство", 3, 5},
                     {"пути", "B-OBST", "обстоятельство", 6, 10},
                     {"расцветали", "B-SKAZ", "сказуемое", 11, 21},
                     {"весенние", "B-OPR", "определение", 22, 30},
                     {"цветы", "B-PODL", "подлежащее", 31, 36},
                     {"зеленела", "B-SKAZ", "сказуемое", 38, 46},
                     {"трава", "B-PODL", "подлежащее", 47, 52}};
  result.tokens = {"На",       "их",    "пути",     "расцветали",
                   "весенние", "цветы", "зеленела", "трава"};
  result.token_labels = {1, 1, 1, 2, 3, 4, 2, 4};
  data.push_back(result);

  // 2
  result.text = "Вот раздался колокольный звон.";
  result.entities = {{"Вот", "B-OBST", "обстоятельство", 0, 3},
                     {"раздался", "B-SKAZ", "сказуемое", 4, 12},
                     {"колокольный", "B-OPR", "определение", 13, 24},
                     {"звон", "B-PODL", "подлежащее", 25, 29}};
  result.tokens = {"Вот", "раздался", "колокольный", "звон"};
  result.token_labels = {1, 2, 3, 4};
  data.push_back(result);

  // 3
  result.text = "Кай и Герда узнали колокольни родного города.";
  result.entities = {{"Кай", "B-PODL", "подлежащее", 0, 3},
                     {"и", "B-DR", "частица", 4, 5},
                     {"Герда", "B-PODL", "подлежащее", 6, 11},
                     {"узнали", "B-SKAZ", "сказуемое", 12, 18},
                     {"колокольни", "B-DOP", "дополнение", 19, 29},
                     {"родного", "B-OPR", "определение", 30, 37},
                     {"города", "B-DOP", "дополнение", 38, 44}};
  result.tokens = {"Кай",        "и",       "Герда", "узнали",
                   "колокольни", "родного", "города"};
  result.token_labels = {4, 5, 4, 2, 6, 3, 6};
  data.push_back(result);

  // 4
  result.text = "Они поднялись по знакомой лестнице и вошли в комнату.";
  result.entities = {{"Они", "B-PODL", "подлежащее", 0, 3},
                     {"поднялись", "B-SKAZ", "сказуемое", 4, 13},
                     {"по", "B-DR", "частица", 14, 16},
                     {"знакомой", "B-OPR", "определение", 17, 25},
                     {"лестнице", "B-OBST", "обстоятельство", 26, 34},
                     {"и", "B-DR", "частица", 35, 36},
                     {"вошли", "B-SKAZ", "сказуемое", 37, 42},
                     {"в", "B-DR", "частица", 43, 44},
                     {"комнату", "B-OBST", "обстоятельство", 45, 52}};
  result.tokens = {"Они", "поднялись", "по", "знакомой", "лестнице",
                   "и",   "вошли",     "в",  "комнату"};
  result.token_labels = {4, 2, 5, 3, 1, 5, 2, 5, 1};
  data.push_back(result);

  // 5
  result.text = "Здесь ничего не изменилось.";
  result.entities = {{"Здесь", "B-OBST", "обстоятельство", 0, 5},
                     {"ничего", "B-DOP", "дополнение", 6, 12},
                     {"не", "B-DR", "частица", 13, 15},
                     {"изменилось", "B-SKAZ", "сказуемое", 16, 26}};
  result.tokens = {"Здесь", "ничего", "не", "изменилось"};
  result.token_labels = {1, 6, 5, 2};
  data.push_back(result);

  // 6
  result.text = "Цветущие розовые кусты заглядывали с крыши в открытое окошко.";
  result.entities = {{"Цветущие", "B-OPR", "определение", 0, 8},
                     {"розовые", "B-OPR", "определение", 9, 16},
                     {"кусты", "B-PODL", "подлежащее", 17, 22},
                     {"заглядывали", "B-SKAZ", "сказуемое", 23, 35},
                     {"с", "B-DR", "частица", 36, 37},
                     {"крыши", "B-OBST", "обстоятельство", 38, 43},
                     {"в", "B-DR", "частица", 44, 45},
                     {"открытое", "B-OPR", "определение", 46, 54},
                     {"окошко", "B-DOP", "дополнение", 55, 61}};
  result.tokens = {"Цветущие", "розовые", "кусты",    "заглядывали", "с",
                   "крыши",    "в",       "открытое", "окошко"};
  result.token_labels = {3, 3, 4, 2, 5, 1, 5, 3, 6};
  data.push_back(result);

  // 7
  result.text = "Тут же стояли их детские стульчики.";
  result.entities = {{"Тут", "B-OBST", "обстоятельство", 0, 3},
                     {"же", "B-OBST", "обстоятельство", 4, 6},
                     {"стояли", "B-SKAZ", "сказуемое", 7, 13},
                     {"их", "B-OPR", "определение", 14, 16},
                     {"детские", "B-OPR", "определение", 17, 24},
                     {"стульчики", "B-PODL", "подлежащее", 25, 35}};
  result.tokens = {"Тут", "же", "стояли", "их", "детские", "стульчики"};
  result.token_labels = {1, 1, 2, 3, 3, 4};
  data.push_back(result);

  // 8
  result.text = "Кай с Гердой сели каждый на свой и взялись за руки.";
  result.entities = {{"Кай", "B-PODL", "подлежащее", 0, 3},
                     {"с", "B-DR", "частица", 4, 5},
                     {"Гердой", "B-DOP", "дополнение", 6, 12},
                     {"сели", "B-SKAZ", "сказуемое", 13, 17},
                     {"каждый", "B-OPR", "определение", 18, 24},
                     {"на", "B-DR", "частица", 25, 27},
                     {"свой", "B-OBST", "обстоятельство", 28, 32},
                     {"и", "B-DR", "частица", 33, 34},
                     {"взялись", "B-SKAZ", "сказуемое", 35, 42},
                     {"за", "B-DR", "частица", 43, 45},
                     {"руки", "B-DOP", "дополнение", 46, 50}};
  result.tokens = {"Кай",  "с", "Гердой",  "сели", "каждый", "на",
                   "свой", "и", "взялись", "за",   "руки"};
  result.token_labels = {4, 5, 6, 2, 3, 5, 1, 5, 2, 5, 6};
  data.push_back(result);

  // 9
  result.text = "Холодное великолепие чертогов Снежной королевы забылось.";
  result.entities = {{"Холодное", "B-OPR", "определение", 0, 8},
                     {"великолепие", "B-PODL", "подлежащее", 9, 20},
                     {"чертогов", "B-DOP", "дополнение", 21, 29},
                     {"Снежной", "B-OPR", "определение", 30, 37},
                     {"королевы", "B-OPR", "определение", 38, 46},
                     {"забылось", "B-SKAZ", "сказуемое", 47, 55}};
  result.tokens = {"Холодное", "великолепие", "чертогов",
                   "Снежной",  "королевы",    "забылось"};
  result.token_labels = {3, 4, 6, 3, 3, 2};
  data.push_back(result);

  // 10
  result.text = "Они стали совсем взрослыми, но были детьми сердцем и душой.";
  result.entities = {{"Они", "B-PODL", "подлежащее", 0, 3},
                     {"стали", "B-SKAZ", "сказуемое", 4, 9},
                     {"совсем", "B-OBST", "обстоятельство", 10, 16},
                     {"взрослыми", "B-OPR", "определение", 17, 26},
                     {"но", "B-DR", "частица", 27, 29},
                     {"были", "B-SKAZ", "сказуемое", 30, 34},
                     {"детьми", "B-DOP", "дополнение", 35, 42},
                     {"сердцем", "B-OBST", "обстоятельство", 43, 50},
                     {"и", "B-DR", "частица", 51, 52},
                     {"душой", "B-OBST", "обстоятельство", 53, 58}};
  result.tokens = {"Они",  "стали",  "совсем",  "взрослыми", "но",
                   "были", "детьми", "сердцем", "и",         "душой"};
  result.token_labels = {4, 2, 1, 3, 5, 2, 6, 1, 5, 1};
  data.push_back(result);

  // 11
  result.text = "На дворе стояло тёплое благодатное лето.";
  result.entities = {{"На", "B-OBST", "обстоятельство", 0, 2},
                     {"дворе", "B-OBST", "обстоятельство", 3, 8},
                     {"стояло", "B-SKAZ", "сказуемое", 9, 15},
                     {"тёплое", "B-OPR", "определение", 16, 22},
                     {"благодатное", "B-OPR", "определение", 23, 34},
                     {"лето", "B-DOP", "дополнение", 35, 39}};
  result.tokens = {"На", "дворе", "стояло", "тёплое", "благодатное", "лето"};
  result.token_labels = {1, 1, 2, 3, 3, 6};
  data.push_back(result);

  return data;
}

/**
 * @brief Обновляет отображение статистики на интерфейсе.
 * @param stats Структура с готовыми данными для отображения.
 */
void ResultPage::updateStatsDisplay(/*GlobalStats& stats*/) {
  // Проверяем, что виджет был создан (чтобы избежать краша при старте)
  if (!statsWidget)
    return;

  // Обновляем текст каждого лейбла, используя данные из структуры
  labelSentences->setText(
      QString("Предложений: %1").arg(stats.sentences_total));
  labelWords->setText(QString("Слов: %1").arg(stats.words_total));
  labelMembers->setText(
      QString("Членов предложения: %1").arg(stats.members_total));

  labelSubjects->setText(QString("Подлежащих: %1").arg(stats.subjects_total));
  labelPredicates->setText(
      QString("Сказуемых: %1").arg(stats.predicates_total));

  labelDefinitions->setText(
      QString("Определений: %1").arg(stats.definitions_total));
  labelAdditions->setText(QString("Дополнений: %1").arg(stats.additions_total));
  labelAdverbials->setText(
      QString("Обстоятельств: %1").arg(stats.adverbials_total));

  // Обновляем топ-слова (самые популярные)
  // Используем QString::fromStdString для перевода std::string -> QString

  top_subject->setText(
      QString("самое популярное подлежащее: %1")
          .arg(QString::fromStdString(stats.top_subject.first)));
  top_predicate->setText(
      QString("самое популярное сказуемое: %1")
          .arg(QString::fromStdString(stats.top_predicate.first)));
  top_definition->setText(
      QString("самое популярное определение: %1")
          .arg(QString::fromStdString(stats.top_definition.first)));
  top_addition->setText(
      QString("самое популярное дополнение: %1")
          .arg(QString::fromStdString(stats.top_addition.first)));
  top_adverbial->setText(
      QString("самое популярное обстоятельство: %1")
          .arg(QString::fromStdString(stats.top_adverbial.first)));
}
