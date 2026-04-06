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

/*
  изменить в main на это
      std::vector<SentenceResult> data = w.makeData();
      w.setData(data); // Передаем данные в виджет
      w.buildWordRoleMap();
      w.updateCounts();
      w.updateChart();
 */

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
  // void loadTextFromFile(const QString &filename);

  /**
   * @brief Загружает разобранные данные из файла.
   * @param filename Путь к файлу с данными разбора.
   */
  // void loadParsedData(const QString &filename);

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
   * @brief Слот, обрабатывающий выбор чекбокса
   * @param state
   */
  void onCheckboxStateChanged(int state, const QString &role);

public:
  void setData(const std::vector<SentenceResult> &results);
  void setSearchItems(const std::vector<SearchItem> &items);
  std::vector<SentenceResult> makeData();
  void updateStatsDisplay();
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
};

#endif // RESULTPAGE_H
