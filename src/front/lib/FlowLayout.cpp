#include "FlowLayout.hpp"

/**
 * @brief Конструктор с указанием родительского виджета.
 * @param parent Родительский виджет.
 * @param margin Внешние отступы со всех сторон.
 * @param hSpacing Горизонтальный отступ между элементами.
 * @param vSpacing Вертикальный отступ между строками.
 */
FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing) {
  setContentsMargins(margin, margin, margin, margin);
}

/**
 * @brief Конструктор без родительского виджета.
 * @param margin Внешние отступы со всех сторон.
 * @param hSpacing Горизонтальный отступ между элементами.
 * @param vSpacing Вертикальный отступ между строками.
 */
FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing) {
  setContentsMargins(margin, margin, margin, margin);
}

/**
 * @brief Деструктор. Удаляет все добавленные элементы.
 */
FlowLayout::~FlowLayout() {
  QLayoutItem *item;
  while ((item = takeAt(0)))
    delete item;
}

/**
 * @brief Добавляет элемент в компоновку.
 * @param item Добавляемый элемент.
 */
void FlowLayout::addItem(QLayoutItem *item) { m_itemList.append(item); }

/**
 * @brief Возвращает текущий горизонтальный отступ между элементами.
 * @return Горизонтальный отступ в пикселях.
 */
int FlowLayout::horizontalSpacing() const {
  if (m_hSpace >= 0)
    return m_hSpace;
  return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

/**
 * @brief Возвращает текущий вертикальный отступ между строками.
 * @return Вертикальный отступ в пикселях.
 */
int FlowLayout::verticalSpacing() const {
  if (m_vSpace >= 0)
    return m_vSpace;
  return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

/**
 * @brief Возвращает направления, в которых компоновка может расширяться.
 * @return Направления расширения (всегда Qt::Horizontal).
 */
Qt::Orientations FlowLayout::expandingDirections() const {
  return Qt::Horizontal;
}

/**
 * @brief Определяет, зависит ли высота от ширины.
 * @return true, так как высота зависит от ширины.
 */
bool FlowLayout::hasHeightForWidth() const { return true; }

/**
 * @brief Вычисляет высоту компоновки для заданной ширины.
 * @param width Ширина, для которой вычисляется высота.
 * @return Требуемая высота компоновки.
 */
int FlowLayout::heightForWidth(int width) const {
  return doLayout(QRect(0, 0, width, 0), true);
}

/**
 * @brief Возвращает количество элементов в компоновке.
 * @return Количество элементов.
 */
int FlowLayout::count() const { return m_itemList.size(); }

/**
 * @brief Возвращает элемент по индексу.
 * @param index Индекс элемента.
 * @return Указатель на элемент или nullptr, если индекс недействителен.
 */
QLayoutItem *FlowLayout::itemAt(int index) const {
  return m_itemList.value(index);
}

/**
 * @brief Извлекает элемент по индексу из компоновки.
 * @param index Индекс извлекаемого элемента.
 * @return Указатель на извлеченный элемент.
 */
QLayoutItem *FlowLayout::takeAt(int index) {
  if (index >= 0 && index < m_itemList.size())
    return m_itemList.takeAt(index);
  return nullptr;
}

/**
 * @brief Устанавливает геометрию компоновки.
 * @param rect Прямоугольник, в котором располагается компоновка.
 */
void FlowLayout::setGeometry(const QRect &rect) {
  QLayout::setGeometry(rect);
  doLayout(rect, false);
}

/**
 * @brief Возвращает рекомендуемый размер компоновки.
 * @return Рекомендуемый размер.
 */
QSize FlowLayout::sizeHint() const { return minimumSize(); }

/**
 * @brief Возвращает минимальный размер компоновки.
 * @return Минимальный размер.
 */
QSize FlowLayout::minimumSize() const {
  QSize size;
  for (const QLayoutItem *item : m_itemList)
    size = size.expandedTo(item->minimumSize());

  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  size += QSize(left + right, top + bottom);
  return size;
}

/**
 * @brief Выполняет компоновку элементов в заданном прямоугольнике.
 * @param rect Прямоугольник для размещения элементов.
 * @param testOnly Если true, только вычисляет размеры без фактического
 * размещения.
 * @return Высота, занятая элементами.
 */
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();
  int y = effectiveRect.y();
  int lineHeight = 0;

  for (QLayoutItem *item : m_itemList) {
    QWidget *wid = item->widget();
    int spaceX = horizontalSpacing();

    if (spaceX == -1)
      spaceX = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    int spaceY = verticalSpacing();

    if (spaceY == -1)
      spaceY = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

    int nextX = x + item->sizeHint().width() + spaceX;

    if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
      x = effectiveRect.x();
      y = y + lineHeight + spaceY;
      nextX = x + item->sizeHint().width() + spaceX;
      lineHeight = 0;
    }

    if (!testOnly)
      item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

    x = nextX;
    lineHeight = qMax(lineHeight, item->sizeHint().height());
  }
  return y + lineHeight - rect.y() + bottom;
}

/**
 * @brief Возвращает стандартный отступ из стиля виджета.
 * @param pm Пиксельная метрика стиля.
 * @return Значение отступа или -1, если родитель не найден.
 */
int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const {
  QObject *parent = this->parent();
  if (!parent) {
    return -1;
  } else if (parent->isWidgetType()) {
    QWidget *pw = static_cast<QWidget *>(parent);
    return pw->style()->pixelMetric(pm, nullptr, pw);
  } else {
    return static_cast<QLayout *>(parent)->spacing();
  }
}
