#ifndef TEXTMARKUPWIDGET_H
#define TEXTMARKUPWIDGET_H

#include "../../back/bert_onnx_inference.hpp"
#include "../../back/unistring.hpp"

#include <QLabel>
#include <QLayout>
#include <QMap>
#include <QScrollArea>
#include <QWidget>

class FlowLayout;

/**
 * @brief Виджет для отображения текста с разметкой частей речи.
 *
 * Отображает текст, разбивая его на слова и знаки препинания.
 * Каждое слово снабжается подписью, указывающей его роль в предложении
 * (подлежащее, сказуемое и т.д.). Слова и подписи форматируются цветами,
 * заданными пользователем.
 */
class TextMarkupWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Конструктор класса TextMarkupWidget.
   * @param parent Указатель на родительский виджет.
   */
  explicit TextMarkupWidget(QWidget *parent = nullptr);

  /**
   * @brief Устанавливает вектор с данными для отображения.
   * @param results Вектор с данными (текст предложения, слова и роли).
   */
  void setMarkupText(std::vector<SentenceResult> &results);

  /**
   * @brief Устанавливает цвет текста слов.
   * @param color Цвет в формате CSS (например, "#000000").
   */
  void setWordColor(const QString &color);

  /**
   * @brief Устанавливает цвет подписей (ролей предложения).
   * @param color Цвет в формате CSS (например, "#808080").
   */
  void setLabelColor(const QString &color);

  /**
   * @brief Возвращает текущий цвет слов.
   * @return Цвет слов в формате CSS.
   */
  QString wordColor() const;

  /**
   * @brief Возвращает текущий цвет подписей.
   * @return Цвет подписей в формате CSS.
   */
  QString labelColor() const;

  /**
   * @brief Возвращает текущий цвет подсветки.
   * @return Цвет подсветки в формате CSS.
   */
  QString wordBackgroundColor() const;

  /**
   * @brief Устанавливает член предложения для подсветки.
   * @param role - название члена предложения.
   */
  void setHighlightedRole(const QString &role);

  /**
   * @brief Возвращает текущий член предложения для подсветки.
   * @return Название члена предложения.
   */
  QString highlightedRole() const;

private:
  /**
   * @brief Перестраивает отображение текста.
   *
   * Очищает текущее содержимое и заново создает все элементы
   * на основе текущих значений m_text и m_members.
   */
  void rebuild();

  /**
   * @brief Подсвечивает выбранные члены предложения.
   */
  void updateHighlighting();

  /**
   * @brief Создает виджет для знака препинания или другого одиночного символа.
   * @param charStr Строка UTF-8 с символом.
   * @param containerHeight Высота контейнера.
   * @return Указатель на созданный виджет.
   */
  QWidget *createPunctuationWidget(const std::string &charStr,
                                   int containerHeight);

  /**
   * @brief Создает виджет для слова с подписью члена предложения.
   * @param entityText Текст слова.
   * @param shortRole Краткая роль члена предложения.
   * @param fullRole Полная роль члена предложения (для tooltip).
   * @param containerHeight Высота контейнера.
   * @return Указатель на созданный виджет.
   */
  QWidget *createWordWidget(const QString &entityText, const QString &shortRole,
                            const QString &fullRole, int containerHeight);

  /**
   * @brief Обрабатывает промежуток текста между сущностями.
   * @param uniGap Подстрока Unistring с текстом промежутка.
   * @param containerHeight Высота контейнера.
   */
  void processGap(const utf8::Unistring &uniGap, int containerHeight);

  /**
   * @brief Обрабатывает хвост строки после последней сущности.
   * @param uniTail Подстрока Unistring с текстом хвоста.
   * @param containerHeight Высота контейнера.
   */
  void processTail(const utf8::Unistring &uniTail, int containerHeight);

  /**
   * @brief Очищает текущий layout виджета.
   */
  void clearLayout();

  /**
   * @brief Вычисляет высоту контейнера для слов и подписей.
   * @return Высота контейнера в пикселях.
   */
  int calculateContainerHeight();

  /**
   * @brief Создает мапу соответствия полных и кратких ролей.
   * @return Мапа ролей.
   */
  QMap<QString, QString> createRoleMap();

  /**
   * @brief Преобразует байтовое смещение в символьный индекс.
   * @param bytePos Байтовое смещение.
   * @param byteOffsets Вектор смещений байтов.
   * @return Символьный индекс.
   */
  size_t byteToCharIndex(size_t bytePos,
                         const std::vector<size_t> &byteOffsets);

  /**
   * @brief Обрабатывает одно предложение с сущностями.
   * @param sentence Предложение с сущностями.
   * @param containerHeight Высота контейнера.
   * @param roleMap Мапа ролей.
   */
  void processSentence(const SentenceResult &sentence, int containerHeight,
                       const QMap<QString, QString> &roleMap);

  QString m_wordColor;  // Цвет текста слов.
  QString m_labelColor; // Цвет подписей (ролей предложения).

  QScrollArea *m_scrollArea; // Область прокрутки для контента.
  QWidget *m_container;      // Контейнер для элементов компоновки.
  FlowLayout *m_flowLayout;  // Компоновка в виде потока.

  QString m_wordBackgroundColor; // Цвет фона слов
  QString m_highlightedRole;     // Роль, которую нужно выделять (например,
                                 // "подлежащее")

  std::vector<SentenceResult> m_results; // вектор с результатами
};

#endif // TEXTMARKUPWIDGET_H
