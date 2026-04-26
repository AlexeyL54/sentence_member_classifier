#include "InputPage.hpp"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <fstream>

InputPage::InputPage(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Анализатор текста");
  resize(900, 550);

  // Построение интерфейса по шагам
  setupMainLayout();
  setupContentRow();
  setupContentColumn();
  setupIntroText();
  setupInputChoice();
  setupKeyboardPage();
  setupFilePage();
  setupConnections();

  // Сборка: стек в колонку контента, колонку в строку, строку в главный layout
  contentLayout->addWidget(stack, 1);
  contentRow->addWidget(contentColumn, 1);
  mainLayout->addLayout(contentRow, 1);
}

/**
 * @brief Обрабатывает нажатие кнопки «Анализировать» для текста с клавиатуры.
 *
 * Проверяет, что поле ввода не пустое, и только после этого
 * отправляет сигнал перехода к анализу.
 */
void InputPage::onAnalyzeFromKeyboard() {
  if (textInput->toPlainText().trimmed().isEmpty()) {
    QMessageBox::warning(this, "Пустой текст",
                         "Введите текст перед запуском анализа.");
    return;
  }

  const std::string text = textInput->toPlainText().toUtf8().toStdString();
  emit analysisRequested(text);
}

/**
 * @brief Обрабатывает нажатие кнопки «Анализировать» для выбранного файла.
 *
 * Проверяет, что файл выбран и доступен для чтения, затем
 * отправляет сигнал перехода к анализу.
 */
void InputPage::onAnalyzeFromFile() {
  const QString path = filePathEdit->text();
  if (path.isEmpty()) {
    QMessageBox::warning(this, "Файл не выбран",
                         "Укажите текстовый файл кнопкой «Выбрать файл».");
    return;
  }

  std::ifstream in(path.toStdString(), std::ios::binary);
  if (!in) {
    QMessageBox::warning(this, "Ошибка чтения",
                         "Не удалось открыть или прочитать файл.");
    return;
  }

  std::string text((std::istreambuf_iterator<char>(in)),
                   std::istreambuf_iterator<char>());
  emit analysisRequested(text);
}

/**
 * @brief Создаёт главный вертикальный layout окна.
 *
 * Layout привязывается к самому виджету InputPage и задаёт
 * общую вертикальную структуру содержимого окна.
 */
void InputPage::setupMainLayout() {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
}

/**
 * @brief Создаёт горизонтальную строку для контента.
 *
 * В эту строку затем добавляются колонки, в том числе
 * колонка с формой ввода текста.
 */
void InputPage::setupContentRow() {
  contentRow = new QHBoxLayout();
  contentRow->setSpacing(0);
}

/**
 * @brief Создаёт колонку контента с отступами и вертикальным layout.
 *
 * Внутрь этой колонки добавляются поясняющий текст, переключатель
 * способа ввода и стек страниц (клавиатура / файл).
 */
void InputPage::setupContentColumn() {
  contentColumn = new QWidget(this);
  contentLayout = new QVBoxLayout(contentColumn);
  contentLayout->setContentsMargins(24, 24, 24, 24);
  contentLayout->setSpacing(16);
}

/**
 * @brief Создаёт и добавляет поясняющий текст вверху страницы.
 *
 * Текст описывает назначение программы и возможные способы ввода.
 */
void InputPage::setupIntroText() {
  introText = new QLabel("\n\nВы можете ввести текст с клавиатуры или "
                         "выбрать текстовый файл с диска.",
                         contentColumn);
  introText->setWordWrap(true);
  contentLayout->addWidget(introText);
}

/**
 * @brief Создаёт переключатель способа ввода и стек страниц.
 *
 * Добавляет радиокнопки «С клавиатуры» / «ИЗ файла» и инициализирует
 * QStackedWidget, в котором будут располагаться соответствующие страницы.
 */
void InputPage::setupInputChoice() {
  inputChoiceGroup = new QButtonGroup(contentColumn);
  radioKeyboard = new QRadioButton("С клавиатуры", contentColumn);
  radioFile = new QRadioButton("Из файла", contentColumn);
  inputChoiceGroup->addButton(radioKeyboard);
  inputChoiceGroup->addButton(radioFile);

  QVBoxLayout *choiceRow = new QVBoxLayout();
  choiceRow->setSpacing(8);
  choiceRow->addWidget(radioKeyboard);
  choiceRow->addWidget(radioFile);
  contentLayout->addLayout(choiceRow);

  stack = new QStackedWidget(contentColumn);
}

/**
 * @brief Создаёт страницу ввода «С клавиатуры».
 *
 * Страница содержит многострочное поле ввода текста и кнопку
 * «Анализировать» для запуска обработки введённого текста.
 */
void InputPage::setupKeyboardPage() {
  pageKeyboard = new QWidget(contentColumn);
  QVBoxLayout *layoutKeyboard = new QVBoxLayout(pageKeyboard);

  // Рамка: слева поле на всю высоту, справа узкая колонка с «Очистить» — без
  // наложения
  QFrame *textFrame = new QFrame(pageKeyboard);
  textFrame->setFrameShape(QFrame::StyledPanel);
  textFrame->setLineWidth(1);
  QVBoxLayout *frameLay = new QVBoxLayout(textFrame);
  frameLay->setContentsMargins(6, 6, 6, 6);
  frameLay->setSpacing(0);

  QHBoxLayout *inputRow = new QHBoxLayout();
  inputRow->setSpacing(8);

  textInput = new QPlainTextEdit(textFrame);
  textInput->setPlaceholderText(QStringLiteral("Введите текст для анализа..."));
  textInput->setMinimumHeight(120);
  // Одна видимая граница — у внешнего QFrame; у самого поля рамку не рисуем
  textInput->setFrameShape(QFrame::NoFrame);
  textInput->setLineWidth(0);
  textInput->setStyleSheet(QStringLiteral(
      "QPlainTextEdit { border: none; padding: 2px; background: transparent; }"
      "QPlainTextEdit:focus { border: none; outline: none; }"));
  inputRow->addWidget(textInput, 1);

  btnClearKeyboard = new QToolButton(textFrame);
  btnClearKeyboard->setText(QStringLiteral("Очистить"));
  btnClearKeyboard->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnClearKeyboard->setAutoRaise(true);
  btnClearKeyboard->setCursor(Qt::PointingHandCursor);
  btnClearKeyboard->setToolTip(
      QStringLiteral("Очистить текст и сбросить путь к выбранному файлу"));
  inputRow->addWidget(btnClearKeyboard, 0, Qt::AlignTop);

  frameLay->addLayout(inputRow, 1);

  layoutKeyboard->addWidget(textFrame, 1);

  // Кнопка запуска анализа при вводе с клавиатуры
  btnAnalyzeKeyboard =
      new QPushButton(QStringLiteral("Анализировать"), pageKeyboard);
  btnAnalyzeKeyboard->setCursor(Qt::PointingHandCursor);
  layoutKeyboard->addWidget(btnAnalyzeKeyboard);

  stack->addWidget(pageKeyboard);
}

/**
 * @brief Создаёт страницу ввода «С файла».
 *
 * Страница содержит поле для отображения пути к файлу и две кнопки:
 * «Выбрать файл» и «Анализировать» выбранный файл.
 */
void InputPage::setupFilePage() {
  pageFile = new QWidget(contentColumn);
  QVBoxLayout *layoutFile = new QVBoxLayout(pageFile);

  // Путь и очистка в одной строке — кнопка визуально «в поле»
  QHBoxLayout *pathRow = new QHBoxLayout();
  pathRow->setSpacing(6);
  filePathEdit = new QLineEdit(pageFile);
  filePathEdit->setReadOnly(true);
  filePathEdit->setPlaceholderText(QStringLiteral("Файл не выбран"));
  pathRow->addWidget(filePathEdit, 1);

  btnClearFile = new QToolButton(pageFile);
  btnClearFile->setText(QStringLiteral("×"));
  btnClearFile->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnClearFile->setAutoRaise(true);
  btnClearFile->setCursor(Qt::PointingHandCursor);
  btnClearFile->setToolTip(
      QStringLiteral("Очистить путь к файлу и введённый текст"));
  btnClearFile->setFixedWidth(28);
  pathRow->addWidget(btnClearFile, 0, Qt::AlignVCenter);
  layoutFile->addLayout(pathRow);

  // Кнопки выбора файла и анализа
  QHBoxLayout *fileButtonsRow = new QHBoxLayout();
  btnSelectFile = new QPushButton(QStringLiteral("Выбрать файл"), pageFile);
  btnSelectFile->setCursor(Qt::PointingHandCursor);
  btnAnalyzeFile = new QPushButton(QStringLiteral("Анализировать"), pageFile);
  btnAnalyzeFile->setCursor(Qt::PointingHandCursor);
  fileButtonsRow->addWidget(btnSelectFile);
  fileButtonsRow->addWidget(btnAnalyzeFile);
  layoutFile->addLayout(fileButtonsRow);

  stack->addWidget(pageFile);
}

/**
 * @brief Настраивает соединения сигналов и слотов.
 *
 * Подключает обработчик выбора файла и переключения страниц стека
 * при изменении состояния радиокнопок, а также задаёт начальное состояние.
 */
void InputPage::setupConnections() {
  // По нажатию «Выбрать файл» — диалог выбора, путь пишем в filePathEdit
  connect(btnSelectFile, &QPushButton::clicked, this, [this]() {
    const QString initialDir =
        lastOpenedDir_.isEmpty() ? QDir::homePath() : lastOpenedDir_;
    QString path = QFileDialog::getOpenFileName(
        this, "Выберите файл", initialDir,
        "Текстовые файлы (*.txt);;Все файлы (*.*)");
    if (!path.isEmpty()) {
      // Сразу проверяем, что файл реально доступен для чтения до нажатия
      // «Анализировать».
      std::ifstream in(path.toStdString(), std::ios::binary);
      if (!in) {
        QMessageBox::warning(this, "Ошибка чтения",
                             "Не удалось открыть выбранный файл для чтения.");
        if (filePathEdit)
          filePathEdit->clear();
        return;
      }

      filePathEdit->setText(path);
      lastOpenedDir_ = QFileInfo(path).absolutePath();
    }
  });

  // Переключение страницы стека при выборе радиокнопки
  connect(radioKeyboard, &QRadioButton::toggled, this, [this](bool checked) {
    if (checked)
      stack->setCurrentIndex(0);
  });
  connect(radioFile, &QRadioButton::toggled, this, [this](bool checked) {
    if (checked)
      stack->setCurrentIndex(1);
  });

  // Начальное состояние: «С клавиатуры», первая страница стека
  radioKeyboard->setChecked(true);
  stack->setCurrentIndex(0);

  connect(btnAnalyzeKeyboard, &QPushButton::clicked, this,
          &InputPage::onAnalyzeFromKeyboard);
  connect(btnAnalyzeFile, &QPushButton::clicked, this,
          &InputPage::onAnalyzeFromFile);

  auto clearKeyboardText = [this]() {
    if (textInput)
      textInput->clear();
  };
  auto clearSelectedFilePath = [this]() {
    if (filePathEdit)
      filePathEdit->clear();
  };
  connect(btnClearKeyboard, &QToolButton::clicked, this, clearKeyboardText);
  connect(btnClearFile, &QToolButton::clicked, this, clearSelectedFilePath);
}
