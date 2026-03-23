#ifndef RESULTPAGE_H
#define RESULTPAGE_H

#include <QButtonGroup>
#include <QTextEdit>
#include <QWidget>
#include <MainWindow.hpp>

/*
class ResultPage : public QWidget {
  Q_OBJECT
private:
signals:
};

*/

#include <QMainWindow>
#include <QTextEdit>
#include <QCheckBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>

class TextMarkupWidget;

class ResultPage : public QMainWindow
{
    Q_OBJECT

    TextMarkupWidget *widgetText;

    QWidget *leftWidget;
    QTextEdit *textEdit;
    QWidget *rightWidget;
    // В классе MainWindow добавляем массив лейблов для отображения количества
    QLabel *countLabels[6]; // По одному для каждого типа члена предложения
    // В классе MainWindow объявляем массив прогрессбаров
    QProgressBar *bars[6]; // По одному для каждого типа члена предложения
    QTextEdit *detailsTextEdit; // <-- НОВОЕ ПОЛЕ для вывода информации

    QPushButton *btnSave;
    QPushButton *btnSearch;


    // 1. Добавляем определение структуры сюда или подключаем внешний файл
    struct SentenceParts {
        QVector<QString> subject;
        QVector<QString> predicate;
        QVector<QString> object;
        QVector<QString> attribute;
        QVector<QString> adverbial;
        QVector<QString> other;
    } parts;

     QMap<QString, QString> members;
     QString fullText;

public:
    ResultPage(QWidget *parent = nullptr);

     void loadTextFromFile(const QString &filename);
     void loadParsedData(const QString &filename);   // Новая функция для загрузки данных
     void updateCounts();
     void updateChart();

     void buildWordRoleMap();
     void readTextFromFile(const QString &filename);

};

/*******************************************************/

/**
 * @brief Вспомогательный класс компоновщика с автоматическим переносом
 * элементов.
 *
 * Обеспечивает перенос дочерних виджетов на новую строку при нехватке места.
 */
class FlowLayout : public QLayout {
  Q_OBJECT

public:
  /**
   * @brief Конструктор с указанием родительского виджета.
   * @param parent Родительский виджет.
   * @param margin Отступы со всех сторон.
   * @param hSpacing Горизонтальный отступ между элементами.
   * @param vSpacing Вертикальный отступ между элементами.
   */
  explicit FlowLayout(QWidget *parent = nullptr, int margin = 5,
                      int hSpacing = 5, int vSpacing = 5);

  /**
   * @brief Конструктор без родительского виджета.
   * @param margin Отступы со всех сторон.
   * @param hSpacing Горизонтальный отступ между элементами.
   * @param vSpacing Вертикальный отступ между элементами.
   */
  explicit FlowLayout(int margin = 5, int hSpacing = 5, int vSpacing = 5);

  /**
   * @brief Деструктор.
   */
  ~FlowLayout();

  /**
   * @brief Добавляет элемент в компоновщик.
   * @param item Добавляемый элемент.
   */
  void addItem(QLayoutItem *item) override;

  /**
   * @brief Возвращает горизонтальный отступ между элементами.
   * @return Значение отступа в пикселях.
   */
  int horizontalSpacing() const;

  /**
   * @brief Возвращает вертикальный отступ между элементами.
   * @return Значение отступа в пикселях.
   */
  int verticalSpacing() const;

  /**
   * @brief Возвращает направления расширения компоновщика.
   * @return Всегда Qt::Horizontal.
   */
  Qt::Orientations expandingDirections() const override;

  /**
   * @brief Показывает, зависит ли высота от ширины.
   * @return Всегда true.
   */
  bool hasHeightForWidth() const override;

  /**
   * @brief Возвращает высоту, необходимую для заданной ширины.
   * @param width Доступная ширина.
   * @return Необходимая высота.
   */
  int heightForWidth(int width) const override;

  /**
   * @brief Возвращает количество элементов в компоновщике.
   * @return Количество элементов.
   */
  int count() const override;

  /**
   * @brief Возвращает элемент по индексу.
   * @param index Индекс элемента.
   * @return Указатель на элемент или nullptr.
   */
  QLayoutItem *itemAt(int index) const override;

  /**
   * @brief Удаляет и возвращает элемент по индексу.
   * @param index Индекс элемента.
   * @return Указатель на удаленный элемент.
   */
  QLayoutItem *takeAt(int index) override;

  /**
   * @brief Устанавливает геометрию компоновщика.
   * @param rect Прямоугольник для размещения элементов.
   */
  void setGeometry(const QRect &rect) override;

  /**
   * @brief Возвращает рекомендуемый размер.
   * @return Рекомендуемый размер.
   */
  QSize sizeHint() const override;

  /**
   * @brief Возвращает минимальный размер.
   * @return Минимальный размер.
   */
  QSize minimumSize() const override;

private:
  /**
   * @brief Выполняет расстановку элементов.
   * @param rect Доступная область.
   * @param testOnly Если true, только вычисляет высоту без установки геометрии.
   * @return Общая использованная высота.
   */
  int doLayout(const QRect &rect, bool testOnly) const;

  /**
   * @brief Вычисляет стандартный отступ на основе стиля.
   * @param pm Тип отступа.
   * @return Значение отступа.
   */
  int smartSpacing(QStyle::PixelMetric pm) const;

  QList<QLayoutItem *> m_itemList; // Список элементов
  int m_hSpace;                    // Горизонтальный отступ
  int m_vSpace;                    // Вертикальный отступ
};

/**
 * @brief Основной виджет для отображения текста с разметкой членов предложения.
 *
 * Позволяет отобразить текст, разбитый на слова, с подписями (членами
 * предложения) под каждым словом. Поддерживает настройку цветов для слов и
 * подписей.
 */
class TextMarkupWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Конструктор.
   * @param parent Родительский виджет.
   */
  explicit TextMarkupWidget(QWidget *parent = nullptr);

  /**
   * @brief Устанавливает текст и его разметку.
   * @param text Исходный текст (разбивается на слова по пробелам).
   * @param members Словарь: слово -> член предложения.
   */
  void setMarkupText(const QString &text,
                     const QMap<QString, QString> &members);

  /**
   * @brief Устанавливает цвет текста (самих слов).
   * @param color Цвет в формате CSS (например, "#FFFFFF").
   */
  void setWordColor(const QString &color);

  /**
   * @brief Устанавливает цвет подписей (членов предложения).
   * @param color Цвет в формате CSS (например, "#AAAAAA").
   */
  void setLabelColor(const QString &color);

  /**
   * @brief Возвращает текущий цвет слов.
   * @return Цвет слов.
   */
  QString wordColor() const;

  /**
   * @brief Возвращает текущий цвет подписей.
   * @return Цвет подписей.
   */
  QString labelColor() const;

private:
  /**
   * @brief Обновляет отображение (перестраивает виджеты слов).
   */
  void rebuild();

  QString m_wordColor;              // Текущий цвет слов
  QString m_labelColor;             // Текущий цвет подписей
  QString m_text;                   // Исходный текст
  QMap<QString, QString> m_members; // Разметка слов

  QScrollArea *m_scrollArea; // Область прокрутки
  QWidget *m_container;      // Контейнер для слов
  FlowLayout *m_flowLayout;  // Layout с переносом слов
};

#endif // RESULTPAGE_H
