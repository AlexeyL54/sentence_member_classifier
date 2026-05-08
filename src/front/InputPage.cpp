#include "InputPage.hpp"
#include "qmessagebox.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <fstream>

namespace {

/**
 * @brief Проверяет, имеет ли файл расширение .txt.
 * @param path Путь к файлу.
 * @return true, если расширение файла равно "txt" (без учёта регистра).
 */
bool isFileExtension(const char *extension, const QString &path) {
  return QFileInfo(path).suffix().compare(QLatin1String(extension),
                                          Qt::CaseInsensitive) == 0;
}

/**
 * @brief Проверяет наличие кириллических символов в строке.
 * @param text Проверяемая строка.
 * @return true, если строка содержит символы кириллицы.
 */
bool containsCyrillic(const QString &text) {
  for (const QChar &ch : text) {
    const ushort code = ch.unicode();
    // Основной диапазон кириллицы + буквы Ё/ё
    if ((code >= 0x0400 && code <= 0x04FF) || code == 0x0401 ||
        code == 0x0451) {
      return true;
    }
  }
  return false;
}

uint8_t bytes_to_encode_symbol(const std::string &str) {
  const unsigned char ch = static_cast<const unsigned char>(str[0]);

  if ((ch & 0b10000000) == 0) { // 0xxxxxxxx
    return 1;
  } else if ((ch & 0b11100000) == 0b11000000) { // 110xxxxx
    return 2;
  } else if ((ch & 0b11110000) == 0b11100000) { // 1110xxxx
    return 3;
  } else if ((ch & 0b11111000) == 0b11110000) { // 11110xxx
    return 4;
  } else {
    return 0;
  }
}

/**
 * @brief Форматирует размер файла в мегабайтах с двумя знаками после запятой.
 * @param bytes Размер в байтах.
 * @return Строка с размером в МБ.
 */
QString formatFileSize(qint64 bytes) {
  return QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f',
                         2);
}

} // namespace

/**
 * @brief Конструктор страницы ввода.
 * @param parent Родительский виджет (по умолчанию nullptr).
 */
InputPage::InputPage(QWidget *parent) : QWidget(parent) {
  setWindowTitle("Анализатор текста");
  resize(900, 550);
  setupUI();
}

/**
 * @brief Инициализирует всю пользовательскую интерфейс.
 */
void InputPage::setupUI() {
  mainLayout_ = new QVBoxLayout(this);
  mainLayout_->setContentsMargins(0, 0, 0, 0);
  mainLayout_->setSpacing(0);

  contentRow_ = new QHBoxLayout();
  contentRow_->setSpacing(0);

  contentColumn_ = new QWidget(this);
  contentLayout_ = new QVBoxLayout(contentColumn_);
  contentLayout_->setContentsMargins(24, 24, 24, 24);
  contentLayout_->setSpacing(16);

  setupIntroLabel();
  setupInputMethodSelector();
  setupStacks();
  setupKeyboardPage();
  setupFilePage();
  setupConnections();

  contentLayout_->addWidget(stack_, 1);
  contentRow_->addWidget(contentColumn_, 1);
  mainLayout_->addLayout(contentRow_, 1);
}

/**
 * @brief Создаёт поясняющий текст в верхней части страницы.
 */
void InputPage::setupIntroLabel() {
  introLabel_ = new QLabel("\n\nВы можете ввести текст с клавиатуры или "
                           "выбрать текстовый файл с диска.",
                           contentColumn_);
  introLabel_->setWordWrap(true);
  contentLayout_->addWidget(introLabel_);
}

/**
 * @brief Создаёт переключатель между клавиатурой и файлом.
 */
void InputPage::setupInputMethodSelector() {
  inputMethodGroup_ = new QButtonGroup(contentColumn_);
  radioKeyboard_ = new QRadioButton("С клавиатуры", contentColumn_);
  radioFile_ = new QRadioButton("Из файла", contentColumn_);

  inputMethodGroup_->addButton(radioKeyboard_);
  inputMethodGroup_->addButton(radioFile_);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->setSpacing(8);
  layout->addWidget(radioKeyboard_);
  layout->addWidget(radioFile_);
  contentLayout_->addLayout(layout);
}

/**
 * @brief Инициализирует стек страниц.
 */
void InputPage::setupStacks() { stack_ = new QStackedWidget(contentColumn_); }

/**
 * @brief Создаёт страницу ввода с клавиатуры.
 */
void InputPage::setupKeyboardPage() {
  keyboardPage_ = new QWidget(contentColumn_);
  QVBoxLayout *layout = new QVBoxLayout(keyboardPage_);

  // Контейнер с рамкой для текстового поля
  QFrame *container = new QFrame(keyboardPage_);
  container->setFrameShape(QFrame::StyledPanel);
  container->setLineWidth(1);

  QVBoxLayout *containerLayout = new QVBoxLayout(container);
  containerLayout->setContentsMargins(6, 6, 6, 6);
  containerLayout->setSpacing(0);

  // Строка: текстовое поле + кнопка очистки
  QHBoxLayout *inputRow = new QHBoxLayout();
  inputRow->setSpacing(8);

  textInput_ = new QPlainTextEdit(container);
  textInput_->setPlaceholderText("Введите текст для анализа...");
  textInput_->setMinimumHeight(120);
  textInput_->setFrameShape(QFrame::NoFrame);
  textInput_->setStyleSheet(
      "QPlainTextEdit { border: none; padding: 2px; background: transparent; }"
      "QPlainTextEdit:focus { border: none; outline: none; }");

  btnClearKeyboard_ = new QToolButton(container);
  btnClearKeyboard_->setText("Очистить");
  btnClearKeyboard_->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnClearKeyboard_->setAutoRaise(true);
  btnClearKeyboard_->setCursor(Qt::PointingHandCursor);
  btnClearKeyboard_->setToolTip("Очистить текст");

  inputRow->addWidget(textInput_, 1);
  inputRow->addWidget(btnClearKeyboard_, 0, Qt::AlignTop);
  containerLayout->addLayout(inputRow, 1);

  btnAnalyzeKeyboard_ = new QPushButton("Анализировать", keyboardPage_);
  btnAnalyzeKeyboard_->setCursor(Qt::PointingHandCursor);

  layout->addWidget(container, 1);
  layout->addWidget(btnAnalyzeKeyboard_);

  stack_->addWidget(keyboardPage_);
}

/**
 * @brief Создаёт страницу выбора файла.
 */
void InputPage::setupFilePage() {
  filePage_ = new QWidget(contentColumn_);
  QVBoxLayout *layout = new QVBoxLayout(filePage_);

  // Строка отображения пути к файлу
  QHBoxLayout *pathRow = new QHBoxLayout();
  pathRow->setSpacing(6);

  filePathEdit_ = new QLineEdit(filePage_);
  filePathEdit_->setReadOnly(true);
  filePathEdit_->setPlaceholderText("Файл не выбран");

  btnClearFile_ = new QToolButton(filePage_);
  btnClearFile_->setText("×");
  btnClearFile_->setToolButtonStyle(Qt::ToolButtonTextOnly);
  btnClearFile_->setAutoRaise(true);
  btnClearFile_->setCursor(Qt::PointingHandCursor);
  btnClearFile_->setToolTip("Очистить путь к файлу");
  btnClearFile_->setFixedWidth(28);

  pathRow->addWidget(filePathEdit_, 1);
  pathRow->addWidget(btnClearFile_, 0, Qt::AlignVCenter);

  // Строка кнопок
  QHBoxLayout *buttonsRow = new QHBoxLayout();
  btnSelectFile_ = new QPushButton("Выбрать файл", filePage_);
  btnAnalyzeFile_ = new QPushButton("Анализировать", filePage_);
  btnSelectFile_->setCursor(Qt::PointingHandCursor);
  btnAnalyzeFile_->setCursor(Qt::PointingHandCursor);

  buttonsRow->addWidget(btnSelectFile_);
  buttonsRow->addWidget(btnAnalyzeFile_);

  layout->addLayout(pathRow);
  layout->addLayout(buttonsRow);

  stack_->addWidget(filePage_);
}

/**
 * @brief Проверяет корректность выбранного файла.
 * @param path Путь к файлу.
 * @param errorMessage [out] Сообщение об ошибке (если указан и проверка не
 * пройдена).
 * @return true, если файл существует, имеет правильный формат и размер; иначе
 * false.
 */
bool InputPage::validateFile(const QString &path, QString *errorMessage) const {
  QFileInfo info(path);
  QString error;

  if (!info.exists() || !info.isFile()) {
    error = "Выбранный файл не существует или недоступен для чтения.";
  } else if (!isFileExtension(ALLOWED_FILE_EXTENSION, path)) {
    error = QString("Разрешена загрузка только файлов формата .%1.")
                .arg(ALLOWED_FILE_EXTENSION);
  } else if (info.size() > MAX_FILE_SIZE_BYTES) {
    error = QString("Размер файла: %1 МБ.\nМаксимально допустимо: %2 МБ.\n\n"
                    "Выберите файл меньшего размера.")
                .arg(formatFileSize(info.size()))
                .arg(formatFileSize(MAX_FILE_SIZE_BYTES));
  }

  if (errorMessage && !error.isEmpty()) {
    *errorMessage = error;
  }

  return error.isEmpty();
}

/**
 * @brief Читает содержимое файла.
 * @param path Путь к файлу.
 * @return Содержимое файла в виде строки (при ошибке возвращает пустую строку).
 */
QString InputPage::readFileContent(const QString &path) {
  std::ifstream file(path.toStdString(), std::ios::binary);
  if (!file) {
    return QString();
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  if (bytes_to_encode_symbol(content) == 0) {
    QMessageBox::warning(this, "Ошибка кодировки",
                         "Убедитесь, что файл сохранен в кодировке UTF-8.");
    return QString();
  }

  return QString::fromUtf8(content.c_str(), static_cast<int>(content.size()));
}

/**
 * @brief Обновляет отображение пути к файлу и запоминает последнюю директорию.
 * @param path Новый путь к файлу (может быть пустым).
 */
void InputPage::updateFilePathDisplay(const QString &path) {
  filePathEdit_->setText(path);
  if (!path.isEmpty()) {
    lastOpenedDir_ = QFileInfo(path).absolutePath();
  }
}

/**
 * @brief Переключает стек на выбранный способ ввода.
 * @param isKeyboard true для страницы клавиатуры, false для страницы файла.
 */
void InputPage::switchToInputMethod(bool isKeyboard) {
  stack_->setCurrentIndex(isKeyboard ? 0 : 1);
}

/**
 * @brief Обработчик нажатия кнопки «Анализировать» для текста с клавиатуры.
 */
void InputPage::onAnalyzeFromKeyboard() {
  if (textInput_->toPlainText().trimmed().isEmpty()) {
    QMessageBox::warning(this, "Пустой текст",
                         "Введите текст перед запуском анализа.");
    return;
  }

  const std::string text = textInput_->toPlainText().toUtf8().toStdString();
  emit analysisRequested(text);
}

/**
 * @brief Обработчик нажатия кнопки «Анализировать» для выбранного файла.
 */
void InputPage::onAnalyzeFromFile() {
  const QString path = filePathEdit_->text();

  if (path.isEmpty()) {
    QMessageBox::warning(this, "Файл не выбран",
                         "Укажите текстовый файл кнопкой «Выбрать файл».");
    return;
  }

  QString errorMsg;
  if (!validateFile(path, &errorMsg)) {
    QMessageBox::warning(this, "Ошибка файла", errorMsg);
    filePathEdit_->clear();
    return;
  }

  const QString content = readFileContent(path);

  if (content.isEmpty()) {
    QMessageBox::warning(this, "Ошибка чтения",
                         "Не удалось прочитать содержимое файла.");
    return;
  }

  emit analysisRequested(content.toStdString());
}

/**
 * @brief Открывает диалог выбора файла и сохраняет путь.
 */
void InputPage::onSelectFile() {
  const QString initialDir =
      lastOpenedDir_.isEmpty() ? QDir::homePath() : lastOpenedDir_;

  const QString path = QFileDialog::getOpenFileName(
      this, "Выберите файл", initialDir,
      QString("Текстовые файлы (*.%1)").arg(ALLOWED_FILE_EXTENSION));

  if (path.isEmpty()) {
    return;
  }

  if (containsCyrillic(path)) {
    QMessageBox::warning(this, "Предупреждение",
                         QString(CYRILLIC_WARNING).arg(path));
    updateFilePathDisplay(QString());
    return;
  }

  QString errorMsg;
  if (!validateFile(path, &errorMsg)) {
    QMessageBox::warning(this, "Ошибка файла", errorMsg);
    updateFilePathDisplay(QString());
    return;
  }

  updateFilePathDisplay(path);
}

/**
 * @brief Очищает текстовое поле на странице клавиатуры.
 */
void InputPage::onClearKeyboard() {
  if (textInput_) {
    textInput_->clear();
  }
}

/**
 * @brief Очищает путь к файлу на странице файла.
 */
void InputPage::onClearFilePath() {
  if (filePathEdit_) {
    filePathEdit_->clear();
  }
}

/**
 * @brief Настраивает соединения сигналов и слотов.
 */
void InputPage::setupConnections() {
  // Переключение способа ввода
  connect(radioKeyboard_, &QRadioButton::toggled, this, [this](bool checked) {
    if (checked) {
      switchToInputMethod(true);
    }
  });

  connect(radioFile_, &QRadioButton::toggled, this, [this](bool checked) {
    if (checked) {
      switchToInputMethod(false);
    }
  });

  // Кнопки очистки
  connect(btnClearKeyboard_, &QToolButton::clicked, this,
          &InputPage::onClearKeyboard);
  connect(btnClearFile_, &QToolButton::clicked, this,
          &InputPage::onClearFilePath);

  // Кнопки анализа
  connect(btnAnalyzeKeyboard_, &QPushButton::clicked, this,
          &InputPage::onAnalyzeFromKeyboard);
  connect(btnAnalyzeFile_, &QPushButton::clicked, this,
          &InputPage::onAnalyzeFromFile);

  // Выбор файла
  connect(btnSelectFile_, &QPushButton::clicked, this,
          &InputPage::onSelectFile);

  // Начальное состояние: выбрана клавиатура
  radioKeyboard_->setChecked(true);
  switchToInputMethod(true);
}
