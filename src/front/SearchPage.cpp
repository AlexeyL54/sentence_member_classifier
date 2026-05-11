#include "SearchPage.hpp"
#include "qboxlayout.h"
#include "qcombobox.h"
#include "qpushbutton.h"

#include <QHBoxLayout>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {
constexpr int kControlHeight = 38;     /** Единая высота элементов управления */
constexpr int kSearchWidth = 640;      /** Ширина поискового блока */
constexpr int kFilterSpacing = 12;     /** Расстояние между фильтрами */
constexpr int kPageMarginsH = 24;      /** Горизонтальные отступы страницы */
constexpr int kPageMarginsTop = 20;    /** Верхний отступ страницы */
constexpr int kPageMarginsBottom = 24; /** Нижний отступ страницы */
constexpr int kPageSpacing = 14;       /** Межэлементный интервал на странице */
} // namespace

/**
 * @brief Конструктор страницы поиска.
 *
 * Создаёт и настраивает все UI-компоненты и связи сигналов-слотов.
 * Начальное состояние: показаны все члены предложения, сортировка А→Я.
 *
 * @param parent Родительский виджет (по умолчанию nullptr).
 */
SearchPage::SearchPage(QWidget *parent) : QWidget(parent) {
  setupUI();
  setupConnections();

  // Начальное состояние: все члены предложения, сортировка А→Я
  memberFilterCombo_->setCurrentIndex(0);
  onMemberFilterChanged();
  applyCurrentFilters();
}

/**
 * @brief Установить данные для поиска.
 *
 * Заменяет текущий набор данных и немедленно применяет
 * активные фильтры для обновления отображения.
 *
 * @param items Вектор элементов для поиска (обычно из build_search_items()).
 */
void SearchPage::setSearchItems(const std::vector<SearchItem> &items) {
  filterModel_.setItems(items);
  applyCurrentFilters();
}

/**
 * @brief Создать и настроить все UI-компоненты.
 *
 * Выделено в отдельный метод для улучшения читаемости конструктора.
 * Создаёт виджеты и устанавливает layout'ы.
 */
void SearchPage::setupUI() {
  QVBoxLayout *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(kPageMarginsH, kPageMarginsTop, kPageMarginsH,
                                 kPageMarginsBottom);
  rootLayout->setSpacing(kPageSpacing);

  // Создаём все виджеты
  backButton_ = createBackButton();
  searchEdit_ = createSearchField();
  memberFilterCombo_ = createMemberFilterCombo();
  sortCombo_ = createSortCombo();
  resultsList_ = new SearchResultsList(this);
  resultsList_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  // Компонуем: кнопка "Назад" → поиск + фильтры → результаты
  rootLayout->addWidget(backButton_, 0, Qt::AlignLeft);
  rootLayout->addWidget(createContentArea(), 1, Qt::AlignHCenter);
}

/**
 * @brief Создать кнопку "Назад" в верхней части страницы.
 * @return Указатель на созданную кнопку.
 */
QPushButton *SearchPage::createBackButton() {
  QPushButton *btn = new QPushButton(QStringLiteral("Назад"), this);
  btn->setCursor(Qt::PointingHandCursor);
  btn->setMinimumHeight(kControlHeight);
  btn->setMaximumWidth(120);
  return btn;
}

/**
 * @brief Создать поле ввода поискового запроса.
 * @return Указатель на созданное поле ввода.
 */
QLineEdit *SearchPage::createSearchField() {
  QLineEdit *edit = new QLineEdit(this);
  edit->setPlaceholderText(QStringLiteral("Введите слово для поиска..."));
  edit->setFixedWidth(kSearchWidth);
  edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  edit->setMinimumHeight(kControlHeight);
  edit->setClearButtonEnabled(true); // Встроенная кнопка очистки
  return edit;
}

/**
 * @brief Создать выпадающий список фильтрации по членам предложения.
 * @return Указатель на созданный комбобокс.
 */
QComboBox *SearchPage::createMemberFilterCombo() {
  auto *combo = new QComboBox(this);
  combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  combo->setMinimumHeight(kControlHeight);
  combo->setFixedWidth((kSearchWidth - kFilterSpacing) / 2);

  // Первый элемент — показать все
  combo->addItem(QStringLiteral("Все члены предложения"));

  // Остальные элементы из модели данных
  for (const QString &member : SearchFilterCore::kAllMembers) {
    // Первая буква заглавная для красоты
    QString displayText = member;
    displayText[0] = displayText[0].toUpper();
    combo->addItem(displayText);
  }

  return combo;
}

/**
 * @brief Создать выпадающий список выбора режима сортировки.
 * @return Указатель на созданный комбобокс.
 */
QComboBox *SearchPage::createSortCombo() {
  QComboBox *combo = new QComboBox(this);
  combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  combo->setMinimumHeight(kControlHeight);
  combo->setFixedWidth((kSearchWidth - kFilterSpacing) / 2);

  // Добавляем режимы сортировки с данными enum'а
  combo->addItem(QStringLiteral("По алфавиту: А → Я"),
                 SearchFilterCore::AlphabetAsc);
  combo->addItem(QStringLiteral("По алфавиту: Я → А"),
                 SearchFilterCore::AlphabetDesc);
  combo->addItem(QStringLiteral("По частоте: сначала частые"),
                 SearchFilterCore::FrequencyDesc);
  combo->addItem(QStringLiteral("По частоте: сначала редкие"),
                 SearchFilterCore::FrequencyAsc);

  return combo;
}

/**
 * @brief Создать строку с фильтрами (члены предложения + сортировка).
 * @return Указатель на виджет, содержащий фильтры в одной строке.
 */
QWidget *SearchPage::createFilterRow() {
  QWidget *row = new QWidget(this);
  QHBoxLayout *layout = new QHBoxLayout(row);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(kFilterSpacing);

  layout->addWidget(memberFilterCombo_, 0, Qt::AlignLeft | Qt::AlignVCenter);
  layout->addWidget(sortCombo_, 0, Qt::AlignRight | Qt::AlignVCenter);

  return row;
}

/**
 * @brief Создать центральную область с поиском, фильтрами и результатами.
 * @return Указатель на виджет центральной области.
 */
QWidget *SearchPage::createContentArea() {
  QWidget *contentWrap = new QWidget(this);
  contentWrap->setFixedWidth(kSearchWidth);

  QVBoxLayout *layout = new QVBoxLayout(contentWrap);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(searchEdit_, 0, Qt::AlignHCenter);
  layout->addWidget(createFilterRow(), 0);
  layout->addWidget(resultsList_, 1); // Растягивается на всё доступное место

  return contentWrap;
}

/**
 * @brief Установить все соединения сигналов и слотов.
 */
void SearchPage::setupConnections() {
  // Кнопка "Назад"
  connect(backButton_, &QPushButton::clicked, this, &SearchPage::backRequested);

  // Живой поиск при вводе текста
  connect(searchEdit_, &QLineEdit::textChanged, this,
          &SearchPage::onSearchTextChanged);

  // Фильтр по членам предложения
  connect(memberFilterCombo_,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SearchPage::onMemberFilterChanged);

  // Сортировка
  connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SearchPage::onSortModeChanged);
}

/**
 * @brief Обработчик изменения текста в поле поиска.
 *
 * Реализует "живой поиск" — фильтрация применяется при каждом
 * изменении текста без необходимости нажимать Enter.
 *
 * @param text Текущий текст из поля поиска.
 */
void SearchPage::onSearchTextChanged(const QString &text) {
  Q_UNUSED(text);
  applyCurrentFilters();
}

/**
 * @brief Обработчик изменения выбранного члена предложения.
 *
 * Обновляет список выбранных членов и переприменяет фильтры.
 */
void SearchPage::onMemberFilterChanged() {
  const int currentIndex = memberFilterCombo_->currentIndex();

  if (currentIndex <= 0) {
    // Выбран пункт "Все члены предложения"
    selectedMembers_ = SearchFilterCore::kAllMembers;
  } else {
    // Выбран конкретный член предложения
    // В комбобоксе первый элемент — "Все", остальные соответствуют kAllMembers
    const int memberIndex = currentIndex - 1;
    if (memberIndex >= 0 &&
        memberIndex < SearchFilterCore::kAllMembers.size()) {
      selectedMembers_ =
          QStringList{SearchFilterCore::kAllMembers[memberIndex]};
    }
  }

  applyCurrentFilters();
}

/**
 * @brief Обработчик изменения режима сортировки.
 *
 * @param index Новый индекс выбранного элемента в комбобоксе сортировки.
 */
void SearchPage::onSortModeChanged(int index) {
  Q_UNUSED(index);
  applyCurrentFilters();
}

/**
 * @brief Применить текущие фильтры и обновить отображение.
 *
 * Основной метод обновления интерфейса. Собирает текущие значения
 * фильтров, передаёт их в SearchFilterModel и обновляет SearchResultsList.
 */
void SearchPage::applyCurrentFilters() {
  // Собираем текущие параметры фильтрации
  const QString searchText = searchEdit_ ? searchEdit_->text() : QString();

  const auto sortMode = static_cast<SearchFilterCore::SortMode>(
      sortCombo_->currentData().toInt());

  // Применяем фильтры через модель
  std::vector<SearchItem> filteredResults =
      filterModel_.applyFilters(searchText, selectedMembers_, sortMode);

  // Обновляем отображение
  resultsList_->setItems(filteredResults);
}
