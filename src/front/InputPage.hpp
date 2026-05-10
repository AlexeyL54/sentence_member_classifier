#ifndef INPUTPAGE_HPP
#define INPUTPAGE_HPP

#include <string>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief Страница ввода текста для анализа.
 *
 * Предоставляет два способа ввода текста:
 * - Ввод с клавиатуры в многострочное поле.
 * - Выбор текстового файла (.txt) с диска.
 *
 * После ввода текста пользователь может запустить его синтаксический анализ.
 */
class InputPage : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Конструктор страницы ввода.
   * @param parent Родительский виджет (по умолчанию nullptr).
   */
  explicit InputPage(QWidget *parent = nullptr);

signals:
  /**
   * @brief Сигнал, испускаемый при готовности текста для анализа.
   * @param text Текст в кодировке UTF-8, который требуется проанализировать.
   */
  void analysisRequested(const std::string &text);

private slots:

  /**
   * @brief Обработчик нажатия кнопки «Анализировать» для текста с клавиатуры.
   */
  void onAnalyzeFromKeyboard();

  /**
   * @brief Обработчик нажатия кнопки «Анализировать» для выбранного файла.
   */
  void onAnalyzeFromFile();

  /**
   * @brief Открывает диалог выбора файла и сохраняет путь.
   */
  void onSelectFile();

  /**
   * @brief Очищает текстовое поле на странице клавиатуры.
   */
  void onClearKeyboard();

  /**
   * @brief Очищает путь к файлу на странице файла.
   */
  void onClearFilePath();

private:
  /**
   * @brief Инициализирует всю пользовательскую интерфейс.
   */
  void setupUI();

  /**
   * @brief Создаёт поясняющий текст в верхней части страницы.
   */
  void setupIntroLabel();

  /**
   * @brief Создаёт переключатель между клавиатурой и файлом.
   */
  void setupInputMethodSelector();

  /**
   * @brief Инициализирует стек страниц.
   */
  void setupStacks();

  /**
   * @brief Создаёт страницу ввода с клавиатуры.
   */
  void setupKeyboardPage();

  /**
   * @brief Создаёт страницу выбора файла.
   */
  void setupFilePage();

  /**
   * @brief Настраивает соединения сигналов и слотов.
   */
  void setupConnections();

  /**
   * @brief Очищает текст от проблемных символов Юникода.
   *
   * Удаляет управляющие символы, мягкие переносы, невидимые символы,
   * оставляя только читаемый текст с базовой пунктуацией.
   *
   * @param text Исходный текст
   * @return Очищенный текст, пригодный для анализа
   */
  QString sanitizeText(const QString &text);

  /**
   * @brief Проверяет текст на наличие только допустимых символов после очистки.
   *
   * @param text Текст для проверки
   * @param originalLength Исходная длина (для статистики)
   * @return true если текст содержит достаточно символов для анализа
   */
  bool isTextValidForAnalysis(const QString &text, int originalLength) const;

  /**
   * @brief Проверяет корректность выбранного файла.
   * @param path Путь к файлу.
   * @param errorMessage [out] Сообщение об ошибке (если указан и проверка не
   * пройдена).
   * @return true, если файл существует, имеет правильный формат и размер; иначе
   * false.
   */
  bool validateFile(const QString &path, QString *errorMessage = nullptr) const;

  /**
   * @brief Читает содержимое файла.
   * @param path Путь к файлу.
   * @return Содержимое файла в виде строки (при ошибке возвращает пустую
   * строку).
   */
  QString readFileContent(const QString &path);

  /**
   * @brief Обновляет отображение пути к файлу и запоминает последнюю
   * директорию.
   * @param path Новый путь к файлу (может быть пустым).
   */
  void updateFilePathDisplay(const QString &path);

  /**
   * @brief Переключает стек на выбранный способ ввода.
   * @param isKeyboard true для страницы клавиатуры, false для страницы файла.
   */
  void switchToInputMethod(bool isKeyboard);

  /** @brief Максимально допустимый размер файла для анализа (300 КБ). */
  static constexpr qint64 MAX_FILE_SIZE_BYTES = 300 * 1024;

  /** @brief Разрешённое расширение файла. */
  static constexpr const char *ALLOWED_FILE_EXTENSION = "txt";

  /** @brief Текст предупреждения о кириллице в пути к файлу. */
  static constexpr const char *CYRILLIC_WARNING =
      "Путь к файлу содержит кириллические символы:\n%1\n\n"
      "Это может вызвать проблемы с чтением файла.";

  QVBoxLayout *mainLayout_ = nullptr; // Главный вертикальный layout окна
  QHBoxLayout *contentRow_ = nullptr; // Строка для колонки с контентом
  QWidget *contentColumn_ = nullptr;  // Колонка, содержащая основные элементы
  QVBoxLayout *contentLayout_ = nullptr; // Layout внутри колонки контента

  QLabel *introLabel_ = nullptr; // Поясняющий текст вверху страницы
  QButtonGroup *inputMethodGroup_ =
      nullptr;                            // Группа кнопок выбора способа ввода
  QRadioButton *radioKeyboard_ = nullptr; // Радиокнопка «С клавиатуры»
  QRadioButton *radioFile_ = nullptr;     // Радиокнопка «Из файла»

  QStackedWidget *stack_ = nullptr; // Стек страниц (клавиатура / файл)
  QWidget *keyboardPage_ = nullptr; // Страница ввода с клавиатуры
  QWidget *filePage_ = nullptr;     // Страница выбора файла

  // Элементы страницы клавиатуры
  QPlainTextEdit *textInput_ = nullptr;     // Поле ввода многострочного текста
  QToolButton *btnClearKeyboard_ = nullptr; // Кнопка очистки текстового поля
  QPushButton *btnAnalyzeKeyboard_ = nullptr; // Анализ текста с клавиатуры

  // Элементы страницы файла
  QLineEdit *filePathEdit_ = nullptr;     // Поле выбранного пути
  QToolButton *btnClearFile_ = nullptr;   // Очистка пути к файлу
  QPushButton *btnSelectFile_ = nullptr;  // Выбора файла
  QPushButton *btnAnalyzeFile_ = nullptr; // Анализ выбранного файла

  QString lastOpenedDir_; // Последняя открытая директория для диалога выбора
};

#endif // INPUTPAGE_HPP
