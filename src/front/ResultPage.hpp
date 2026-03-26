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
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

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
   * @brief Загружает текст из файла для отображения.
   * @param filename Путь к файлу с текстом.
   */
  void loadTextFromFile(const QString &filename);

  /**
   * @brief Загружает разобранные данные из файла.
   * @param filename Путь к файлу с данными разбора.
   */
  void loadParsedData(const QString &filename);

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
};

#endif // RESULTPAGE_H
