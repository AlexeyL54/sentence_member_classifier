#ifndef TEXTMARKUPWIDGET_H
#define TEXTMARKUPWIDGET_H

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
   * @brief Устанавливает текст и карту соответствия слов их ролям.
   * @param text Текст для отображения.
   * @param members Карта соответствия: слово -> роль в предложении.
   */
  void setMarkupText(const QString &text,
                     const QMap<QString, QString> &members);

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

  /*
   *
   */
  //void setWordBackgroundColor(const QString &color);
  QString wordBackgroundColor() const;

  void setHighlightedRole(const QString &role);
     QString highlightedRole() const;

private:
  /**
   * @brief Перестраивает отображение текста.
   *
   * Очищает текущее содержимое и заново создает все элементы
   * на основе текущих значений m_text и m_members.
   */
  void rebuild();

  void updateHighlighting();

  QString m_wordColor;              // Цвет текста слов.
  QString m_labelColor;             // Цвет подписей (ролей предложения).
  QString m_text;                   // Отображаемый текст.
  QMap<QString, QString> m_members; // Карта: слово -> его роль.

  QScrollArea *m_scrollArea; // Область прокрутки для контента.
  QWidget *m_container;      // Контейнер для элементов компоновки.
  FlowLayout *m_flowLayout;  // Компоновка в виде потока.

  QString m_wordBackgroundColor; // Цвет фона слов
  QString m_highlightedRole; // Роль, которую нужно выделять (например, "подлежащее")

};

#endif // TEXTMARKUPWIDGET_H
