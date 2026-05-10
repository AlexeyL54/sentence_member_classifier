#ifndef RESULTPAGE_H
#define RESULTPAGE_H

#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStyle>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

#include "../back/bert_onnx_inference.hpp"
#include "../back/statistics.hpp"
#include "lib/TextMarkupWidget.hpp"
#include "qobject.h"

/**
 * @brief Главное окно для отображения результатов анализа предложений.
 *
 * Класс отвечает за отображение текста с подсветкой частей речи,
 * статистику по частям предложения и предоставляет интерфейс для поиска.
 */
class ResultPage : public QMainWindow {
  Q_OBJECT

public:
  /**
   * @brief Конструктор класса ResultPage.
   * @param parent Указатель на родительский виджет.
   */
  ResultPage(QWidget *parent = nullptr);

  /**
   * @brief Обновляет отображение количества элементов в каждой категории.
   */
  void updateCounts();

  /**
   * @brief Обновляет графическое отображение (прогресс-бары) статистики.
   */
  void updateChart();

  /**
   * @brief Обновляет все отображения на интерфейсе.
   */
  void refreshDisplay();

  /**
   * @brief Структура для хранения частей предложения.
   */
  struct SentenceParts {
    int subject = 0;   // Подлежащие
    int predicate = 0; // Сказуемые
    int object = 0;    // Дополнения
    int attribute = 0; // Определения
    int adverbial = 0; // Обстоятельства
    int other = 0;     // Другие части
  };

signals:
  /**
   * @brief Сигнал, испускаемый при запросе поиска.
   */
  void searchRequested();

  /**
   * @brief Сигнал, испускаемый при запросе нового анализа.
   */
  void newAnalysisRequested();

private slots:
  /**
   * @brief Слот, обрабатывающий нажатие кнопки поиска.
   */
  void onSearchClicked();

  /**
   * @brief Слот, обрабатывающий нажатие кнопки Анализ.
   */
  void onAnalyzeClicked();

  /**
   * @brief Слот, обрабатывающий нажатие кнопки Сохранить.
   */
  void SaveClicked();

  /**
   * @brief Слот, обрабатывающий выбор чекбокса
   * @param state - признак установки/снятия чекбокса
   * @param role - название члена предложения
   */
  void onCheckboxStateChanged(int state, const QString &role);

public:
  /**
   * @brief Устанавливает значения для отображения.
   * @param results Вектор данных для отображения.
   */
  void setData(const std::vector<SentenceResult> &results);

  /**
   * @brief Устанавливает значения элепментов для сохраниния в файл.
   * @param items Вектор элементов для сохранения в файл.
   */
  void setSearchItems(const std::vector<SearchItem> &items);

  /**
   * @brief Обновляет данные в разделе статистики
   */
  void updateStatsDisplay();

  /**
   * @brief Устанавливает данные статистики.
   * @param statistics Структура с данными статистики.
   */
  void setGlobalStats(const GlobalStats stats);

private:
  /**
   * @brief Инициализирует левую часть интерфейса с текстовым полем и кнопками.
   * @param leftLayout Layout для размещения элементов.
   */
  void initLeftPanel(QVBoxLayout *leftLayout);

  /**
   * @brief Создает чекбокс для отображения члена предложения.
   * @param parent Родительский виджет.
   * @param name Название члена предложения.
   * @param role Роль члена предложения.
   * @param index Индекс чекбокса в массиве.
   * @return Указатель на созданный чекбокс.
   */
  QCheckBox *createPartCheckbox(QWidget *parent, const QString &name,
                                const QString &role, int index);

  /**
   * @brief Инициализирует правую панель с чекбоксами членов предложения.
   * @param rightLayout Layout для размещения элементов.
   */
  void initRightPanel(QVBoxLayout *rightLayout);

  /**
   * @brief Создает и инициализирует виджет общей статистики.
   * @param parent Родительский виджет.
   * @return Указатель на созданный layout статистики.
   */
  QVBoxLayout *createGeneralStatsWidget(QWidget *parent);

  /**
   * @brief Создает виджет популярных членов предложения.
   * @param parent Родительский виджет.
   * @return Указатель на созданный виджет.
   */
  QWidget *createPopularPartsWidget(QWidget *parent);

  /**
   * @brief Инициализирует панель статистики.
   * @param rightLayout Layout правой панели для добавления статистики.
   */
  void initStatsPanel(QVBoxLayout *rightLayout);

  /**
   * @brief Подсчитывает количество членов предложения по типам.
   * @param results Результаты анализа предложений.
   * @param parts Структура для хранения подсчитанных значений.
   */
  void countSentenceParts(const std::vector<SentenceResult> &results,
                          SentenceParts &parts);

  /**
   * @brief Проверяет наличие кириллических символов в пути.
   * @param path Путь для проверки.
   * @return true, если путь содержит кириллицу, иначе false.
   */
  bool pathContainsCyrillic(const QString &path);

  /**
   * @brief Показывает предупреждение о кириллических символах в пути.
   * @param path Путь с кириллицей.
   * @return true, если пользователь подтвердил продолжение, иначе false.
   */
  bool showCyrillicWarning(const QString &path);

  /**
   * @brief Проверяет существование файлов для сохранения.
   * @param path Путь к директории.
   * @param searchFile Имя файла поиска.
   * @param reviewFile Имя файла статистики.
   * @param searchFileExists Флаг существования файла поиска (выходной
   * параметр).
   * @param reviewFileExists Флаг существования файла статистики (выходной
   * параметр).
   */
  void checkExistingFiles(const QString &path, const std::string &searchFile,
                          const std::string &reviewFile, bool &searchFileExists,
                          bool &reviewFileExists);

  /**
   * @brief Показывает предупреждение о перезаписи существующих файлов.
   * @param searchFileExists Флаг существования файла поиска.
   * @param reviewFileExists Флаг существования файла статистики.
   * @return true, если пользователь подтвердил перезапись, иначе false.
   */
  bool showOverwriteWarning(bool searchFileExists, bool reviewFileExists);

  /**
   * @brief Показывает результат сохранения файлов.
   * @param path Путь сохранения.
   * @param searchFileExists Флаг существования файла поиска после сохранения.
   * @param reviewFileExists Флаг существования файла статистики после
   * сохранения.
   * @return true, если сохранение успешно, иначе false.
   */
  bool showSaveResult(const QString &path, bool searchFileExists,
                      bool reviewFileExists);

  /**
   * @brief Создает и настраивает центральный виджет.
   * @return Указатель на центральный виджет.
   */
  QWidget *setupCentralWidget();

  /**
   * @brief Настраивает левую панель интерфейса.
   * @param centralWidget Центральный виджет.
   * @param mainLayout Главный layout.
   */
  void setupLeftPanel(QWidget *centralWidget, QHBoxLayout *mainLayout);

  /**
   * @brief Настраивает правую панель интерфейса.
   * @param centralWidget Центральный виджет.
   * @param mainLayout Главный layout.
   */
  void setupRightPanel(QWidget *centralWidget, QHBoxLayout *mainLayout);

  /**
   * @brief Создает инструкцию на правой панели.
   * @param rightLayout Layout правой панели.
   */
  void setupInstructionLabel(QVBoxLayout *rightLayout);

  /**
   * @brief Настраивает соединения сигналов и слотов.
   */
  void setupConnections();

private:
  const std::string SEARCH_FILE = "list.html";
  const std::string REVIEW_FILE = "statistics.html";

  QString lastOpenedDir_;

  TextMarkupWidget *widgetText;
  QWidget *leftWidget;
  QTextEdit *textEdit;
  QWidget *rightWidget;
  QLabel *countLabels[6];
  QProgressBar *bars[6];
  QTextEdit *detailsTextEdit;
  QPushButton *btnSave;
  QPushButton *btnSearch;
  QPushButton *btnAnalize;

  QVector<QCheckBox *> m_checkBoxes;

  GlobalStats stats;

  std::vector<SearchItem> search_items;

  SentenceParts parts;

  bool isSaved = false;

  std::vector<SentenceResult> m_results;

  // статистика
  QLabel *labelSentences;
  QLabel *labelWords;
  QLabel *labelMembers;
  QLabel *labelSubjects;
  QLabel *labelPredicates;
  QLabel *labelDefinitions;
  QLabel *labelAdditions;
  QLabel *labelAdverbials;
  QLabel *labelOthers;
  QLabel *top_subject;
  QLabel *top_predicate;
  QLabel *top_definition;
  QLabel *top_addition;
  QLabel *top_adverbial;
  QLabel *top_other;
  QWidget *statsWidget;
};

#endif // RESULTPAGE_H
