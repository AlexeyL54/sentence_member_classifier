#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QStackedWidget>
#include <QWidget>
#include <memory>
#include <vector>

#include "InputPage.hpp"
#include "LoadingPage.hpp"
#include "ResultPage.hpp"
#include "SearchPage.hpp"

/**
 * @brief Главное окно приложения "Анализатор текста"
 * @details Управляет навигацией между страницами и обработкой результатов
 * анализа
 */
class MainWindow : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Конструктор главного окна
   * @param parent Родительский виджет
   */
  explicit MainWindow(QWidget *parent = nullptr);

  /**
   * @brief Деструктор главного окна
   */
  ~MainWindow();

protected:
  /**
   * @brief Обработчик события закрытия окна
   * @param event Событие закрытия
   * @details Проверяет наличие несохранённых результатов и предлагает их
   * сохранить
   */
  void closeEvent(QCloseEvent *event) override;

private slots:
  /**
   * @brief Слот для обработки запроса на анализ текста
   * @param text Текст для анализа
   * @details Выполняет проверку наличия модели и запускает процесс анализа
   */
  void onAnalyzeRequested(const std::string &text);

  /**
   * @brief Слот для обработки запроса на поиск
   * @details Переключает стековый виджет на страницу поиска
   */
  void onSearchRequested();

  /**
   * @brief Слот для возврата к странице результатов
   * @details Обновляет отображение счетчиков и диаграммы, переключается на
   * страницу результатов
   */
  void onBackToResultRequested();

  /**
   * @brief Слот для запроса нового анализа
   * @details Очищает результаты анализа и переключается на страницу ввода
   * текста
   */
  void onNewAnalysisRequested();

private:
  std::map<int, std::pair<std::string, std::string>> labels; // Метки сущностей
  std::shared_ptr<SimpleTokenizer> tokenizer;      // Токенизатор текста
  std::unique_ptr<onnx_infer::BertNerModel> model; // ONNX модель BERT
  std::unique_ptr<BertOnnxInference> inferer;      // Инференс движок
  std::vector<SentenceResult> results;             // Результаты анализа
  bool hasUnsavedResults = false;                  // Флаг несохранённых данных

  QStackedWidget *stackedWidget = nullptr; // Стек для переключения страниц
  InputPage *inputPage = nullptr;          // Страница ввода текста
  ResultPage *resultPage = nullptr;        // Страница результатов
  SearchPage *searchPage = nullptr;        // Страница поиска
  LoadingPage *loadingPage = nullptr;      // Страница загрузки

  QVector<SearchItem> searchItems_; // Элементы для поиска

  /**
   * @brief Настройка пользовательского интерфейса
   * @details Создает и настраивает все страницы, добавляет их в стековый виджет
   */
  void setupUI();

  /**
   * @brief Настройка сигнально-слотовых соединений
   * @details Устанавливает соединения между сигналами страниц и слотами
   * главного окна
   */
  void setupConnections();

  /**
   * @brief Проверка наличия необходимых директорий
   * @details Проверяет существование директории ../model, при отсутствии
   * показывает ошибку
   */
  void checkAppDirs();

  /**
   * @brief Инициализация всех моделей (обёртка)
   * @details Вызывает методы загрузки меток, токенизатора и модели
   */
  void initializeModels();

  /**
   * @brief Загрузка меток из конфигурации
   * @details Загружает метки сущностей из файла конфигурации модели
   */
  void setLabels();

  /**
   * @brief Инициализация токенизатора
   * @details Создает токенизатор на основе файла словаря
   */
  void setTokenizer();

  /**
   * @brief Загрузка ONNX модели
   * @details Загружает модель BERT NER из файла
   */
  void setModel();

  /**
   * @brief Отображение ошибки парсинга
   * @details Показывает сообщение об ошибке и возвращается на страницу ввода
   */
  void showParsingError();

  /**
   * @brief Обновить вывод результатов анализа текста
   * @details Строит элементы поиска и статистику, обновляет страницы
   * результатов
   */
  void updatePagesWithResults();

  /**
   * @brief Запуск процесса анализа текста
   * @param text Текст для анализа
   * @details Разбивает текст на предложения и обрабатывает каждое с обновлением
   * прогресса
   */
  void processAnalysis(const std::string &text);

  /**
   * @brief Запрос подтверждения при обнаружении кириллицы в пути
   * @param path Путь, содержащий кириллицу
   * @return true если пользователь подтвердил продолжение, false если отменил
   */
  bool confirmCyrillicPath(const QString &path);

  /**
   * @brief Сохранение результатов анализа при закрытии окна
   * @return true если сохранение прошло успешно или не требуется, false если
   * отменено
   */
  bool saveResultsOnClose();

  /**
   * @brief Проверка пути на наличие кириллицы
   * @param path Путь для проверки
   * @return true если путь содержит кириллические символы
   */
  bool hasCyrillicPath(const QString &path) const;

  /**
   * @brief Запрос подтверждения перезаписи существующих файлов
   * @param searchFilePath Путь к файлу поиска
   * @param reviewFilePath Путь к файлу обзора
   * @return true если пользователь подтвердил перезапись
   */
  bool confirmFileOverwrite(const QString &searchFilePath,
                            const QString &reviewFilePath);

  /**
   * @brief Сохранение файлов результатов
   * @param directoryPath Путь для сохранения
   * @return true если сохранение успешно
   */
  bool saveResultFiles(const QString &directoryPath);
};

#endif // MAINWINDOW_H
