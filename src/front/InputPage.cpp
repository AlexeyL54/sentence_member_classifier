#include "InputPage.hpp"
#include <QDir>
#include <QFileDialog>

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
  introText = new QLabel(
      "Эта программа предназначена для анализа русского текста: поиск членов "
      "предложения и слов, не являющихся членами предложения, с последующей "
      "обработкой результатов.\n\nВы можете ввести текст с клавиатуры или "
      "выбрать текстовый файл с диска.",
      contentColumn);
  introText->setWordWrap(true);
  contentLayout->addWidget(introText);
}

/**
 * @brief Создаёт переключатель способа ввода и стек страниц.
 *
 * Добавляет радиокнопки «С клавиатуры» / «С файла» и инициализирует
 * QStackedWidget, в котором будут располагаться соответствующие страницы.
 */
void InputPage::setupInputChoice() {
  inputChoiceGroup = new QButtonGroup(contentColumn);
  radioKeyboard = new QRadioButton("С клавиатуры", contentColumn);
  radioFile = new QRadioButton("С файла", contentColumn);
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

  textInput = new QPlainTextEdit(contentColumn);
  textInput->setPlaceholderText("Введите текст для анализа...");
  textInput->setMinimumHeight(120);
  layoutKeyboard->addWidget(textInput);

  // Кнопка запуска анализа при вводе с клавиатуры
  btnAnalyzeKeyboard = new QPushButton("Анализировать", contentColumn);
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

  filePathEdit = new QLineEdit(contentColumn);
  filePathEdit->setReadOnly(true);
  filePathEdit->setPlaceholderText("Файл не выбран");
  layoutFile->addWidget(filePathEdit);

  // Кнопки выбора файла и анализа
  QHBoxLayout *fileButtonsRow = new QHBoxLayout();
  btnSelectFile = new QPushButton("Выбрать файл", contentColumn);
  btnSelectFile->setCursor(Qt::PointingHandCursor);
  btnAnalyzeFile = new QPushButton("Анализировать", contentColumn);
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
    QString path = QFileDialog::getOpenFileName(
        this, "Выберите файл", QDir::homePath(),
        "Текстовые файлы (*.txt);;Все файлы (*.*)");
    if (!path.isEmpty())
      filePathEdit->setText(path);
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

  // ========== ДОБАВЛЯЕМ ПОДКЛЮЧЕНИЕ КНОПОК АНАЛИЗА ==========
  // Сигнал от кнопки на странице "С клавиатуры"
  connect(btnAnalyzeKeyboard, &QPushButton::clicked, this,
          [this]() { emit analyzeRequested(); });

  // Сигнал от кнопки на странице "С файла"
  connect(btnAnalyzeFile, &QPushButton::clicked, this,
          [this]() { emit analyzeRequested(); });
  // ========================================================
}
