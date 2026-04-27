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

signals:
  /**
   * @brief Сигнал, испускаемый при запросе поиска.
   */
  void searchRequested();

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
  //std::vector<SentenceResult> makeData();

  /**
   * @brief Обновляет данные в разделе статистики
   */
  void updateStatsDisplay();

  /**
   * @brief Устанавливает данные статистики.
   * @param statistics Структура с данными статистики.
   */
  void setGloabalStats(const GlobalStats stats);

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

  GlobalStats stats;

  std::vector<SearchItem> search_items;

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
  } parts;

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
