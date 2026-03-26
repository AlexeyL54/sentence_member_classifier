#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QStyle>
#include <QWidget>

/**
 * @brief Класс, реализующий компоновку элементов в виде "потока".
 *
 * Элементы располагаются горизонтально, переносятся на новую строку
 * при достижении правой границы контейнера. Поведение аналогично
 * обтеканию текста или расположению элементов в потоке.
 */
class FlowLayout : public QLayout {
  Q_OBJECT

public:
  /**
   * @brief Конструктор с указанием родительского виджета.
   * @param parent Родительский виджет.
   * @param margin Внешние отступы со всех сторон.
   * @param hSpacing Горизонтальный отступ между элементами.
   * @param vSpacing Вертикальный отступ между строками.
   */
  explicit FlowLayout(QWidget *parent = nullptr, int margin = 5,
                      int hSpacing = 5, int vSpacing = 5);

  /**
   * @brief Конструктор без родительского виджета.
   * @param margin Внешние отступы со всех сторон.
   * @param hSpacing Горизонтальный отступ между элементами.
   * @param vSpacing Вертикальный отступ между строками.
   */
  explicit FlowLayout(int margin = 5, int hSpacing = 5, int vSpacing = 5);

  /**
   * @brief Деструктор. Удаляет все добавленные элементы.
   */
  ~FlowLayout();

  /**
   * @brief Добавляет элемент в компоновку.
   * @param item Добавляемый элемент.
   */
  void addItem(QLayoutItem *item) override;

  /**
   * @brief Возвращает текущий горизонтальный отступ между элементами.
   * @return Горизонтальный отступ в пикселях.
   */
  int horizontalSpacing() const;

  /**
   * @brief Возвращает текущий вертикальный отступ между строками.
   * @return Вертикальный отступ в пикселях.
   */
  int verticalSpacing() const;

  /**
   * @brief Возвращает направления, в которых компоновка может расширяться.
   * @return Направления расширения (всегда Qt::Horizontal).
   */
  Qt::Orientations expandingDirections() const override;

  /**
   * @brief Определяет, зависит ли высота от ширины.
   * @return true, так как высота зависит от ширины.
   */
  bool hasHeightForWidth() const override;

  /**
   * @brief Вычисляет высоту компоновки для заданной ширины.
   * @param width Ширина, для которой вычисляется высота.
   * @return Требуемая высота компоновки.
   */
  int heightForWidth(int width) const override;

  /**
   * @brief Возвращает количество элементов в компоновке.
   * @return Количество элементов.
   */
  int count() const override;

  /**
   * @brief Возвращает элемент по индексу.
   * @param index Индекс элемента.
   * @return Указатель на элемент или nullptr, если индекс недействителен.
   */
  QLayoutItem *itemAt(int index) const override;

  /**
   * @brief Извлекает элемент по индексу из компоновки.
   * @param index Индекс извлекаемого элемента.
   * @return Указатель на извлеченный элемент.
   */
  QLayoutItem *takeAt(int index) override;

  /**
   * @brief Устанавливает геометрию компоновки.
   * @param rect Прямоугольник, в котором располагается компоновка.
   */
  void setGeometry(const QRect &rect) override;

  /**
   * @brief Возвращает рекомендуемый размер компоновки.
   * @return Рекомендуемый размер.
   */
  QSize sizeHint() const override;

  /**
   * @brief Возвращает минимальный размер компоновки.
   * @return Минимальный размер.
   */
  QSize minimumSize() const override;

private:
  /**
   * @brief Выполняет компоновку элементов в заданном прямоугольнике.
   * @param rect Прямоугольник для размещения элементов.
   * @param testOnly Если true, только вычисляет размеры без фактического
   * размещения.
   * @return Высота, занятая элементами.
   */
  int doLayout(const QRect &rect, bool testOnly) const;

  /**
   * @brief Возвращает стандартный отступ из стиля виджета.
   * @param pm Пиксельная метрика стиля.
   * @return Значение отступа или -1, если родитель не найден.
   */
  int smartSpacing(QStyle::PixelMetric pm) const;

  QList<QLayoutItem *> m_itemList; // Список элементов компоновки.
  int m_hSpace;                    // Горизонтальный отступ между элементами.
  int m_vSpace;                    // Вертикальный отступ между строками.
};

#endif // FLOWLAYOUT_H
