#include "InputPage.hpp"
#include "qlogging.h"
#include "qmessagebox.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>

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
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QString();
  }

  QByteArray data = file.readAll();

  // Пробуем декодировать как UTF-8
  QString content = QString::fromUtf8(data);

  // Проверяем, успешно ли декодировалось
  if (content.isEmpty() && !data.isEmpty()) {
    QMessageBox::warning(this, "Ошибка кодировки",
                         "Файл не в кодировке UTF-8 или содержит ошибки.");
    return QString();
  }

  // Дополнительная проверка: если были заменяющие символы
  if (content.contains(QChar::ReplacementCharacter)) {
    QMessageBox::warning(
        this, "Ошибка кодировки",
        "Файл содержит некорректные UTF-8 последовательности.");
    return QString();
  }

  return content;
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

QString InputPage::sanitizeText(const QString &text) {
  if (text.isEmpty()) {
    return QString();
  }

  QString cleaned = text;

  // Удаляем все, что находится между [ и ], включая сами скобки
  // Используем нежадный захват (.*?) чтобы не захватывать больше, чем нужно
  static const QRegularExpression bracketsContent("\\[.*?\\]");
  cleaned.remove(bracketsContent);

  // Удаляем мягкий перенос (U+00AD)
  cleaned.remove(QChar(0x00AD));

  // Заменяем неразрывный пробел (U+00A0) на обычный пробел
  cleaned.replace(QChar(0x00A0), QChar(' '));

  // Удаляем символы нулевой ширины:
  // U+200B (zero-width space)
  // U+200C (zero-width non-joiner)
  // U+200D (zero-width joiner)
  cleaned.remove(QChar(0x200B));
  cleaned.remove(QChar(0x200C));
  cleaned.remove(QChar(0x200D));

  // Удаляем символы направления текста (U+202A, U+202B, U+202C, U+202D, U+202E)
  for (ushort c = 0x202A; c <= 0x202E; ++c) {
    cleaned.remove(QChar(c));
  }

  // Категории символов, которые МЫ ОСТАВЛЯЕМ:
  // \\p{L}  - любая буква (русские, английские, любые алфавитные)
  // \\p{N}  - любая цифра
  // \\s     - любой пробельный символ (пробел, табуляция, перевод строки)
  // \\p{P}  - любой знак пунктуации
  static const QRegularExpression invalidChars("[^\\p{L}\\p{N}\\s\\p{P}]");
  cleaned.remove(invalidChars);

  // Заменяем множественные пробелы на один
  cleaned.replace(QRegularExpression("\\s+"), " ");

  // Удаляем пробелы перед знаками пунктуации (опционально)
  cleaned.replace(QRegularExpression("\\s+([.,!?;:])"), "\\1");

  // Добавляем пробел после знаков пунктуации, если его нет (опционально)
  cleaned.replace(QRegularExpression("([.,!?;:])(\\S)"), "\\1 \\2");

  cleaned = cleaned.trimmed();

  return cleaned;
}

bool InputPage::isTextValidForAnalysis(const QString &text,
                                       int originalLength) const {
  if (text.isEmpty()) {
    QMessageBox::warning(
        const_cast<InputPage *>(this), "Текст пуст после очистки",
        QString(
            "Исходный текст содержал %1 символов, но после удаления "
            "недопустимых символов\n"
            "результат оказался пуст. Возможно, файл содержит только "
            "специальные символы.\n\n"
            "Пожалуйста, проверьте исходный текст или используйте другой файл.")
            .arg(originalLength));
    return false;
  }

  // Предупреждаем, если очистка удалила много символов
  if (originalLength > 0 && text.length() < originalLength / 2) {
    QMessageBox::StandardButton reply = QMessageBox::warning(
        const_cast<InputPage *>(this), "Много символов удалено",
        QString(
            "После очистки текста было удалено %1 символов (%.1f%% от "
            "исходного объема).\n"
            "Возможно, исходный текст содержит много недопустимых символов.\n\n"
            "Продолжить анализ с очищенным текстом?")
            .arg(originalLength - text.length())
            .arg(100.0 * (originalLength - text.length()) / originalLength),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
      return false;
    }
  }

  return true;
}

// Измените onAnalyzeFromKeyboard:
void InputPage::onAnalyzeFromKeyboard() {
  QString rawText = textInput_->toPlainText();

  if (rawText.trimmed().isEmpty()) {
    QMessageBox::warning(this, "Пустой текст",
                         "Введите текст перед запуском анализа.");
    return;
  }

  // Очищаем текст от проблемных символов
  QString cleanedText = sanitizeText(rawText);

  // Проверяем валидность очищенного текста
  if (!isTextValidForAnalysis(cleanedText, rawText.length())) {
    return;
  }

  // Если очистка изменила текст, показываем предупреждение (опционально)
  if (rawText != cleanedText) {
    qDebug() << "Text was sanitized. Original length:" << rawText.length()
             << "Cleaned length:" << cleanedText.length();
    // Можно показать информационное сообщение:
    // QMessageBox::information(this, "Текст очищен",
    //    "Из текста были удалены недопустимые символы.\nАнализ будет продолжен
    //    с очищенным текстом.");
  }

  emit analysisRequested(cleanedText.toStdString());
}

// Измените onAnalyzeFromFile:
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

  QString content = readFileContent(path);

  if (content.isEmpty()) {
    QMessageBox::warning(this, "Ошибка чтения",
                         "Не удалось прочитать содержимое файла.");
    return;
  }

  // Очищаем текст от проблемных символов
  QString cleanedContent = sanitizeText(content);

  // Проверяем валидность очищенного текста
  if (!isTextValidForAnalysis(cleanedContent, content.length())) {
    return;
  }

  // Логируем очистку
  if (content != cleanedContent) {
    qDebug() << "File content was sanitized. Removed:"
             << (content.length() - cleanedContent.length()) << "chars";
  }

  emit analysisRequested(cleanedContent.toStdString());
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
