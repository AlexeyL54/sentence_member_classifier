#include "ResultPage.hpp"
#include "../back/save_result.hpp"
#include "lib/TextMarkupWidget.hpp"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSizePolicy>
#include <QString>
#include <QTextStream>
#include <QVector>

/**
 * @brief Инициализирует левую часть интерфейса с текстовым полем и кнопками.
 * @param leftLayout Layout для размещения элементов.
 */
void ResultPage::initLeftPanel(QVBoxLayout *leftLayout) {
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
}

/**
 * @brief Создает чекбокс для отображения члена предложения.
 * @param parent Родительский виджет.
 * @param name Название члена предложения.
 * @param role Роль члена предложения.
 * @param index Индекс чекбокса в массиве.
 * @return Указатель на созданный чекбокс.
 */
QCheckBox *ResultPage::createPartCheckbox(QWidget *parent, const QString &name,
                                          const QString &role, int index) {
  QWidget *rowWidget = new QWidget(parent);
  QHBoxLayout *hLayout = new QHBoxLayout(rowWidget);
  hLayout->setContentsMargins(0, 0, 0, 0);
  hLayout->setSpacing(8);

  QCheckBox *cb = new QCheckBox(name, rowWidget);
  cb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  cb->setFixedWidth(150);

#ifdef WIN32
  connect(cb, &QCheckBox::stateChanged, this,
          [this, role](int state) { onCheckboxStateChanged(state, role); });
#else
  connect(cb, &QCheckBox::checkStateChanged, this,
          [this, role](int state) { onCheckboxStateChanged(state, role); });
#endif

  countLabels[index] = new QLabel("0", rowWidget);
  countLabels[index]->setMinimumWidth(35);
  countLabels[index]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  countLabels[index]->setStyleSheet("font-weight: bold;");

  bars[index] = new QProgressBar(rowWidget);
  bars[index]->setTextVisible(false);
  bars[index]->setRange(0, 1);
  bars[index]->setFixedWidth(150);
  bars[index]->setFixedHeight(20);

  hLayout->addWidget(cb);
  hLayout->addWidget(countLabels[index]);
  hLayout->addWidget(bars[index]);
  hLayout->addStretch();

  QVBoxLayout *rightLayout = qobject_cast<QVBoxLayout *>(parent->layout());
  if (rightLayout) {
    rightLayout->addWidget(rowWidget);
  }

  return cb;
}

/**
 * @brief Инициализирует правую панель с чекбоксами членов предложения.
 * @param rightLayout Layout для размещения элементов.
 */
void ResultPage::initRightPanel(QVBoxLayout *rightLayout) {
  QStringList partNames = {"Подлежащее",  "Сказуемое",      "Дополнение",
                           "Определение", "Обстоятельство", "Другое"};
  QStringList roles = {"подл.", "сказ.", "доп.", "опр.", "обст.", "др."};

  for (int i = 0; i < partNames.size(); ++i) {
    QCheckBox *cb =
        createPartCheckbox(rightWidget, partNames[i], roles.at(i), i);
    m_checkBoxes.append(cb);
  }
}

/**
 * @brief Создает и инициализирует виджет общей статистики.
 * @param parent Родительский виджет.
 * @return Указатель на созданный layout статистики.
 */
QVBoxLayout *ResultPage::createGeneralStatsWidget(QWidget *parent) {
  QVBoxLayout *statsLayout = new QVBoxLayout(parent);
  statsLayout->setContentsMargins(15, 25, 15, 15);
  statsLayout->setSpacing(6);
  parent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

  QLabel *statsTitle = new QLabel("Статистика", parent);
  statsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
  statsLayout->addWidget(statsTitle);

  labelSentences = new QLabel("Предложений: 0", parent);
  labelWords = new QLabel("Слов: 0", parent);
  labelMembers = new QLabel("Членов предложения: 0", parent);
  labelSubjects = new QLabel("Подлежащих: 0", parent);
  labelPredicates = new QLabel("Сказуемых: 0", parent);
  labelDefinitions = new QLabel("Определений: 0", parent);
  labelAdditions = new QLabel("Дополнений: 0", parent);
  labelAdverbials = new QLabel("Обстоятельств: 0", parent);
  labelOthers = new QLabel("Других: 0", parent);

  statsLayout->addWidget(labelSentences);
  statsLayout->addWidget(labelWords);
  statsLayout->addWidget(labelMembers);
  statsLayout->addWidget(labelSubjects);
  statsLayout->addWidget(labelPredicates);
  statsLayout->addWidget(labelDefinitions);
  statsLayout->addWidget(labelAdditions);
  statsLayout->addWidget(labelAdverbials);
  statsLayout->addWidget(labelOthers);

  return statsLayout;
}

/**
 * @brief Создает виджет популярных членов предложения.
 * @param parent Родительский виджет.
 * @return Указатель на созданный виджет.
 */
QWidget *ResultPage::createPopularPartsWidget(QWidget *parent) {
  QWidget *popularWidget = new QWidget(parent);
  QVBoxLayout *popularLayout = new QVBoxLayout(popularWidget);
  popularLayout->setContentsMargins(0, 10, 0, 0);
  popularLayout->setSpacing(6);

  QLabel *popularTitle =
      new QLabel("Самые популярные члены предложения", popularWidget);
  popularTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
  popularLayout->addWidget(popularTitle);

  top_subject = new QLabel("Подлежащее:", popularWidget);
  top_predicate = new QLabel("Сказуемое:", popularWidget);
  top_definition = new QLabel("Определение:", popularWidget);
  top_addition = new QLabel("Дополнение:", popularWidget);
  top_adverbial = new QLabel("Обстоятельство:", popularWidget);
  top_other = new QLabel("Другое:", popularWidget);

  popularLayout->addWidget(top_subject);
  popularLayout->addWidget(top_predicate);
  popularLayout->addWidget(top_definition);
  popularLayout->addWidget(top_addition);
  popularLayout->addWidget(top_adverbial);
  popularLayout->addWidget(top_other);

  return popularWidget;
}

/**
 * @brief Инициализирует панель статистики.
 * @param rightLayout Layout правой панели для добавления статистики.
 */
void ResultPage::initStatsPanel(QVBoxLayout *rightLayout) {
  statsWidget = new QWidget(rightWidget);
  QVBoxLayout *statsLayout = createGeneralStatsWidget(statsWidget);

  QWidget *popularWidget = createPopularPartsWidget(statsWidget);
  statsLayout->addWidget(popularWidget);

  statsLayout->addStretch();
  rightLayout->addWidget(statsWidget);
}

/**
 * @brief Подсчитывает количество членов предложения по типам.
 * @param results Результаты анализа предложений.
 * @param parts Структура для хранения подсчитанных значений.
 */
void ResultPage::countSentenceParts(const std::vector<SentenceResult> &results,
                                    SentenceParts &parts) {
  for (const SentenceResult &sentence : results) {
    for (size_t i = 0; i < sentence.entities.size(); ++i) {
      const std::string &type = sentence.entities[i].type_ru;

      if (type == "подлежащее") {
        parts.subject++;
      } else if (type == "сказуемое") {
        parts.predicate++;
      } else if (type == "дополнение") {
        parts.object++;
      } else if (type == "определение") {
        parts.attribute++;
      } else if (type == "обстоятельство") {
        parts.adverbial++;
      } else {
        parts.other++;
      }
    }
  }
}

/**
 * @brief Проверяет наличие кириллических символов в пути.
 * @param path Путь для проверки.
 * @return true, если путь содержит кириллицу, иначе false.
 */
bool ResultPage::pathContainsCyrillic(const QString &path) {
  for (const QChar &ch : path) {
    if (ch.unicode() >= 0x0400 && ch.unicode() <= 0x04FF) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Показывает предупреждение о кириллических символах в пути.
 * @param path Путь с кириллицей.
 * @return true, если пользователь подтвердил продолжение, иначе false.
 */
bool ResultPage::showCyrillicWarning(const QString &path) {
  QMessageBox::warning(
      this, "Предупреждение",
      "Путь к директории содержит кириллические символы:\n" + path +
          "\n\nЭто может вызвать проблемы с сохранением файлов.");

  QMessageBox msgBox(this);
  msgBox.setWindowTitle("Подтверждение");
  msgBox.setText("Вы действительно хотите продолжить сохранение?");
  msgBox.setIcon(QMessageBox::Question);

  QPushButton *yesButton = msgBox.addButton("Да", QMessageBox::YesRole);
  QPushButton *noButton = msgBox.addButton("Нет", QMessageBox::NoRole);

  msgBox.exec();

  return msgBox.clickedButton() == yesButton;
}

/**
 * @brief Проверяет существование файлов для сохранения.
 * @param path Путь к директории.
 * @param searchFile Имя файла поиска.
 * @param reviewFile Имя файла статистики.
 * @param searchFileExists Флаг существования файла поиска (выходной параметр).
 * @param reviewFileExists Флаг существования файла статистики (выходной
 * параметр).
 */
void ResultPage::checkExistingFiles(const QString &path,
                                    const std::string &searchFile,
                                    const std::string &reviewFile,
                                    bool &searchFileExists,
                                    bool &reviewFileExists) {
  QString searchFilePath = path + "/" + QString::fromStdString(searchFile);
  QString reviewFilePath = path + "/" + QString::fromStdString(reviewFile);

  searchFileExists = QFile::exists(searchFilePath);
  reviewFileExists = QFile::exists(reviewFilePath);
}

/**
 * @brief Показывает предупреждение о перезаписи существующих файлов.
 * @param searchFileExists Флаг существования файла поиска.
 * @param reviewFileExists Флаг существования файла статистики.
 * @return true, если пользователь подтвердил перезапись, иначе false.
 */
bool ResultPage::showOverwriteWarning(bool searchFileExists,
                                      bool reviewFileExists) {
  QMessageBox warningMsgBox(this);
  warningMsgBox.setWindowTitle("Предупреждение");
  QString warningText = "В выбранной директории уже существуют файлы:\n";

  if (searchFileExists) {
    warningText += "- " + QString::fromStdString(SEARCH_FILE) + "\n";
  }
  if (reviewFileExists) {
    warningText += "- " + QString::fromStdString(REVIEW_FILE) + "\n";
  }

  warningText += "\nЕсли вы продолжите, эти файлы будут перезаписаны.";
  warningMsgBox.setText(warningText);
  warningMsgBox.setIcon(QMessageBox::Warning);

  QPushButton *overwriteButton =
      warningMsgBox.addButton("Продолжить", QMessageBox::YesRole);
  warningMsgBox.addButton("Отмена", QMessageBox::RejectRole);

  warningMsgBox.exec();

  return warningMsgBox.clickedButton() == overwriteButton;
}

/**
 * @brief Показывает результат сохранения файлов.
 * @param path Путь сохранения.
 * @param searchFileExists Флаг существования файла поиска после сохранения.
 * @param reviewFileExists Флаг существования файла статистики после сохранения.
 * @return true, если сохранение успешно, иначе false.
 */
bool ResultPage::showSaveResult(const QString &path, bool searchFileExists,
                                bool reviewFileExists) {
  if (searchFileExists && reviewFileExists) {
    QMessageBox::information(this, "Сохранение",
                             "Результаты успешно сохранены в:\n" + path + "\n" +
                                 "- " + QString::fromStdString(SEARCH_FILE) +
                                 "\n" + "- " +
                                 QString::fromStdString(REVIEW_FILE) + "\n");
    return true;
  } else {
    QString errorMsg = "Ошибка сохранения!\n";
    if (!searchFileExists) {
      errorMsg +=
          "Не удалось сохранить файл: " + QString::fromStdString(SEARCH_FILE) +
          "\n";
    }
    if (!reviewFileExists) {
      errorMsg +=
          "Не удалось сохранить файл: " + QString::fromStdString(REVIEW_FILE) +
          "\n";
    }
    QMessageBox::critical(this, "Ошибка сохранения", errorMsg);
    return false;
  }
}

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
  initLeftPanel(leftLayout);

  // Правая часть: чекбоксы и лейблы с количеством
  rightWidget = new QWidget(centralWidget);
  rightWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
  rightWidget->setFixedWidth(410);

  QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
  rightLayout->setContentsMargins(15, 15, 15, 15);
  rightLayout->setSpacing(12);
  rightLayout->setAlignment(Qt::AlignTop);

  initRightPanel(rightLayout);
  initStatsPanel(rightLayout);

  // Создаем инструкцию
  QLabel *instructionLabel =
      new QLabel("Нажмите кнопку Поиск для получения подробной информации");
  instructionLabel->setStyleSheet("font-style: italic; font-weight: bold;");
  instructionLabel->setAlignment(Qt::AlignLeft);
  instructionLabel->setWordWrap(true);

  rightLayout->addWidget(instructionLabel);
  rightLayout->addStretch();

  mainLayout->addWidget(leftWidget, 1);
  mainLayout->addWidget(rightWidget, 0);

  connect(btnSearch, &QPushButton::clicked, this, &ResultPage::searchRequested);
  connect(btnAnalize, &QPushButton::clicked, this,
          &ResultPage::onAnalyzeClicked);

  connect(btnSave, &QPushButton::clicked, this, &ResultPage::SaveClicked);

  updateStatsDisplay();
}

void ResultPage::setSearchItems(const std::vector<SearchItem> &items) {
  search_items = items;
}

/**
 * @brief Обновляет отображение количества элементов в каждой категории.
 */
void ResultPage::updateCounts() {
  countLabels[0]->setText(QString::number(parts.subject));
  countLabels[1]->setText(QString::number(parts.predicate));
  countLabels[2]->setText(QString::number(parts.object));
  countLabels[3]->setText(QString::number(parts.attribute));
  countLabels[4]->setText(QString::number(parts.adverbial));
  countLabels[5]->setText(QString::number(parts.other));
}

/**
 * @brief Обновляет графическое отображение (прогресс-бары) статистики.
 */
void ResultPage::updateChart() {
  int counts[] = {(parts.subject),   (parts.predicate), (parts.object),
                  (parts.attribute), (parts.adverbial), (parts.other)};

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

void ResultPage::SaveClicked() {
  const QString initialDir =
      lastOpenedDir_.isEmpty() ? QDir::homePath() : lastOpenedDir_;

  QString path = QFileDialog::getExistingDirectory(
      this, "Выберите директорию", initialDir,
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if (!path.isEmpty()) {
    if (pathContainsCyrillic(path)) {
      if (!showCyrillicWarning(path)) {
        return;
      }
    }

    lastOpenedDir_ = path;
    std::string stdPath = path.toStdString();

    bool searchFileExists, reviewFileExists;
    checkExistingFiles(path, SEARCH_FILE, REVIEW_FILE, searchFileExists,
                       reviewFileExists);

    if (searchFileExists || reviewFileExists) {
      // Пользователь отменил сохранение
      if (!showOverwriteWarning(searchFileExists, reviewFileExists)) {
        return;
      }
    }

    saveAnalysis(stdPath, search_items, stats);

    // Проверка, что файлы действительно сохранились
    bool searchFileExistsAfter =
        QFile::exists(path + "/" + QString::fromStdString(SEARCH_FILE));
    bool reviewFileExistsAfter =
        QFile::exists(path + "/" + QString::fromStdString(REVIEW_FILE));

    isSaved =
        showSaveResult(path, searchFileExistsAfter, reviewFileExistsAfter);
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
    widgetText->setMarkupText(m_results);
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
  msgBox.setWindowTitle("Подтверждение");
  msgBox.setText(str);
  msgBox.setIcon(QMessageBox::Question);
  QPushButton *yesButton = msgBox.addButton("Да", QMessageBox::YesRole);
  QPushButton *noButton = msgBox.addButton("Нет", QMessageBox::NoRole);

  msgBox.exec();

  if (msgBox.clickedButton() == yesButton) {
    m_results.clear();

    // Очищаем структуру частей предложения
    parts.subject = 0;
    parts.predicate = 0;
    parts.object = 0;
    parts.attribute = 0;
    parts.adverbial = 0;
    parts.other = 0;

    // Сбрасываем флаг сохранения
    isSaved = false;

    // Снимаем выделение со всех чекбоксов и разблокируем их
    foreach (QCheckBox *box, m_checkBoxes) {
      box->setChecked(false);
      box->setEnabled(true);
    }

    emit newAnalysisRequested();

  } else {
    // Пользователь выбрал "Нет" или закрыл окно
    QMessageBox::information(this, "Отмена", "Анализ отменён.");
  }
}

void ResultPage::onCheckboxStateChanged(int state, const QString &role) {
  QObject *senderObj = sender();
  QCheckBox *currentCheckbox = qobject_cast<QCheckBox *>(senderObj);

  if (!currentCheckbox) {
    return;
  }

  // Если галочку установили
  if (state == Qt::Checked) {
    foreach (QCheckBox *box, m_checkBoxes) {
      if (box != currentCheckbox) {
        box->setEnabled(false);
        box->setChecked(false);
      }
    }
    if (widgetText) {
      widgetText->setHighlightedRole(role);
    }
  }
  // Если галочку сняли (отжали)
  else if (state == Qt::Unchecked) {
    foreach (QCheckBox *box, m_checkBoxes) {
      box->setEnabled(true);
    }

    // Если галочка снята со всех (или мы просто хотим снять выделение)
    if (widgetText) {
      widgetText->setHighlightedRole("");
    }
  }
}

void ResultPage::setData(const std::vector<SentenceResult> &results) {
  m_results = results;

  parts.subject = 0;
  parts.predicate = 0;
  parts.object = 0;
  parts.attribute = 0;
  parts.adverbial = 0;
  parts.other = 0;

  if (m_results.empty()) {
    refreshDisplay();
    return;
  }

  countSentenceParts(m_results, parts);

  if (widgetText) {
    widgetText->setMarkupText(m_results);
  }
}

/**
 * @brief Устанавливает данные статистики.
 * @param statistics Структура с данными статистики.
 */
void ResultPage::setGloabalStats(const GlobalStats statistics) {
  stats = statistics;
}

/**
 * @brief Обновляет отображение статистики на интерфейсе.
 * @param stats Структура с готовыми данными для отображения.
 */
void ResultPage::updateStatsDisplay() {
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
  labelOthers->setText(QString("Других: %1").arg(stats.others_total));

  // Обновляем топ-слова (самые популярные)
  top_subject->setText(
      QString("Подлежащее: %1")
          .arg(QString::fromStdString(stats.top_subject.first)));
  top_predicate->setText(
      QString("Сказуемое: %1")
          .arg(QString::fromStdString(stats.top_predicate.first)));
  top_definition->setText(
      QString("Определение: %1")
          .arg(QString::fromStdString(stats.top_definition.first)));
  top_addition->setText(
      QString("Дополнение: %1")
          .arg(QString::fromStdString(stats.top_addition.first)));
  top_adverbial->setText(
      QString("Обстоятельство: %1")
          .arg(QString::fromStdString(stats.top_adverbial.first)));
  top_other->setText(
      QString("Другое: %1").arg(QString::fromStdString(stats.top_other.first)));
}
