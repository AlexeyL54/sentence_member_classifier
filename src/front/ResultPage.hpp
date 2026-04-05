#ifndef RESULTPAGE_H
#define RESULTPAGE_H

#include <QTableWidget>
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
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "TextMarkupWidget.hpp"

/*
  изменить в main на это
      std::vector<SentenceResult> data = w.makeData();
      w.setData(data); // Передаем данные в виджет
      w.buildWordRoleMap();
      w.updateCounts();
      w.updateChart();
 */


// Entity сложная структура, её нужно определить здесь.
using Entity = std::string;

struct SentenceResult {
    std::string text;                // Исходный текст предложения
    std::vector<Entity> entities;    // Найденные слова
    std::vector<std::string> tokens; // названия членов предложения
    std::vector<int> token_labels;   // Метки (индексы)
};

/**
 * @brief Структура для хранения статистических данных о тексте
 */
struct GlobalStats {
  int sentences_total;   // количество предложений в тексте
  int words_total;       // количество слов в тексте
  int members_total;     // количество членов предложения в тексте
  int subjects_total;    // количество подлежащий в тексте
  int predicates_total;  // количество сказуемых в тексте
  int definitions_total; // количество определений в тексте
  int additions_total;   // количество дополнений в тексте
  int adverbials_total;  // количество обстоятельств в тексте
  std::pair<std::string, int> top_subject;    // самое популярное подлежащее
  std::pair<std::string, int> top_predicate;  // самое популярное сказуемое
  std::pair<std::string, int> top_definition; // самое популярное определение
  std::pair<std::string, int> top_addition;   // самое популярное дополнение
  std::pair<std::string, int> top_adverbial;  // самое популярное обстоятельство
};


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
   * @brief Загружает текст из файла для отображения.
   * @param filename Путь к файлу с текстом.
   */
  //void loadTextFromFile(const QString &filename);

  /**
   * @brief Загружает разобранные данные из файла.
   * @param filename Путь к файлу с данными разбора.
   */
  //void loadParsedData(const QString &filename);

  /**
   * @brief Обновляет отображение количества элементов в каждой категории.
   */
  void updateCounts();

  /**
   * @brief Обновляет графическое отображение (прогресс-бары) статистики.
   */
  void updateChart();

  /**
   * @brief Строит карту соответствия слов их ролям в предложении.
   */
  void buildWordRoleMap();

  /**
   * @brief Считывает текст из файла во внутреннюю переменную.
   * @param filename Путь к файлу с текстом.
   */
  void readTextFromFile(const QString &filename);

  /**
   * @brief Обновляет все отображения на интерфейсе.
   */
  void refreshDisplay();

signals:
  /**
   * @brief Сигнал, испускаемый при запросе поиска.
   */
  void searchRequested();

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
     * @brief Слот, обрабатывающий выбор чекбокса
     * @param state
     */
    void onCheckboxStateChanged(int state, const QString &role);

public:
    void setData(const std::vector<SentenceResult>& results);
    std::vector<SentenceResult> makeData();
    void updateStatsDisplay(/*GlobalStats& stats*/);

private:
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

  /**
   * @brief Структура для хранения частей предложения.
   */
  struct SentenceParts {
    QVector<QString> subject;   // Подлежащие
    QVector<QString> predicate; // Сказуемые
    QVector<QString> object;    // Дополнения
    QVector<QString> attribute; // Определения
    QVector<QString> adverbial; // Обстоятельства
    QVector<QString> other;     // Другие части
  } parts;

  QMap<QString, QString> members; // Карта: слово -> его роль
  QString fullText;               // Полный текст для анализа

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
      QLabel *top_subject;
      QLabel *top_predicate;
      QLabel *top_definition;
      QLabel *top_addition;
      QLabel *top_adverbial;
      QWidget *statsWidget;


      // это здесь временно
      struct GlobalStats {
        int sentences_total = 11;
        int words_total = 81;
        int members_total = 81;
        int subjects_total = 11;
        int predicates_total = 15;
        int definitions_total = 9;
        int additions_total = 16;
        int adverbials_total = 16;
        std::pair<std::string, int> top_subject {"нет",0};
        std::pair<std::string, int> top_predicate{"нет",0};
        std::pair<std::string, int> top_definition{"нет",0};
        std::pair<std::string, int> top_addition{"нет",0};
        std::pair<std::string, int> top_adverbial{"нет",0};
      } stats;
};

#endif // RESULTPAGE_H
