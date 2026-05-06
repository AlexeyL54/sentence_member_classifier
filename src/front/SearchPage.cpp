#include "SearchPage.hpp"

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

SearchPage::SearchPage(QWidget *parent) : QWidget(parent) {
  setupUI();
  setupConnections();

  // Начальное состояние: все члены предложения, сортировка А→Я
  memberFilterCombo_->setCurrentIndex(0);
  onMemberFilterChanged();
  applyCurrentFilters();
}

void SearchPage::setSearchItems(const std::vector<SearchItem> &items) {
  filterModel_.setItems(items);
  applyCurrentFilters();
}

void SearchPage::setupUI() {
  auto *rootLayout = new QVBoxLayout(this);
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

QPushButton *SearchPage::createBackButton() {
  auto *btn = new QPushButton(QStringLiteral("Назад"), this);
  btn->setCursor(Qt::PointingHandCursor);
  btn->setMinimumHeight(kControlHeight);
  btn->setMaximumWidth(120);
  return btn;
}

QLineEdit *SearchPage::createSearchField() {
  auto *edit = new QLineEdit(this);
  edit->setPlaceholderText(QStringLiteral("Введите слово для поиска..."));
  edit->setFixedWidth(kSearchWidth);
  edit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  edit->setMinimumHeight(kControlHeight);
  edit->setClearButtonEnabled(true); // Встроенная кнопка очистки
  return edit;
}

QComboBox *SearchPage::createMemberFilterCombo() {
  auto *combo = new QComboBox(this);
  combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  combo->setMinimumHeight(kControlHeight);
  combo->setFixedWidth((kSearchWidth - kFilterSpacing) / 2);

  // Первый элемент — показать все
  combo->addItem(QStringLiteral("Все члены предложения"));

  // Остальные элементы из модели данных
  for (const auto &member : SearchFilterModel::kAllMembers) {
    // Первая буква заглавная для красоты
    QString displayText = member;
    displayText[0] = displayText[0].toUpper();
    combo->addItem(displayText);
  }

  return combo;
}

QComboBox *SearchPage::createSortCombo() {
  auto *combo = new QComboBox(this);
  combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  combo->setMinimumHeight(kControlHeight);
  combo->setFixedWidth((kSearchWidth - kFilterSpacing) / 2);

  // Добавляем режимы сортировки с данными enum'а
  combo->addItem(QStringLiteral("По алфавиту: А → Я"),
                 SearchFilterModel::AlphabetAsc);
  combo->addItem(QStringLiteral("По алфавиту: Я → А"),
                 SearchFilterModel::AlphabetDesc);
  combo->addItem(QStringLiteral("По частоте: сначала частые"),
                 SearchFilterModel::FrequencyDesc);
  combo->addItem(QStringLiteral("По частоте: сначала редкие"),
                 SearchFilterModel::FrequencyAsc);

  return combo;
}

QWidget *SearchPage::createFilterRow() {
  auto *row = new QWidget(this);
  auto *layout = new QHBoxLayout(row);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(kFilterSpacing);

  layout->addWidget(memberFilterCombo_, 0, Qt::AlignLeft | Qt::AlignVCenter);
  layout->addWidget(sortCombo_, 0, Qt::AlignRight | Qt::AlignVCenter);

  return row;
}

QWidget *SearchPage::createContentArea() {
  auto *contentWrap = new QWidget(this);
  contentWrap->setFixedWidth(kSearchWidth);

  auto *layout = new QVBoxLayout(contentWrap);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  layout->addWidget(searchEdit_, 0, Qt::AlignHCenter);
  layout->addWidget(createFilterRow(), 0);
  layout->addWidget(resultsList_, 1); // Растягивается на всё доступное место

  return contentWrap;
}

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

void SearchPage::onSearchTextChanged(const QString &text) {
  Q_UNUSED(text);
  applyCurrentFilters();
}

void SearchPage::onMemberFilterChanged() {
  const int currentIndex = memberFilterCombo_->currentIndex();

  if (currentIndex <= 0) {
    // Выбран пункт "Все члены предложения"
    selectedMembers_ = SearchFilterModel::kAllMembers;
  } else {
    // Выбран конкретный член предложения
    // В комбобоксе первый элемент — "Все", остальные соответствуют kAllMembers
    const int memberIndex = currentIndex - 1;
    if (memberIndex >= 0 &&
        memberIndex < SearchFilterModel::kAllMembers.size()) {
      selectedMembers_ =
          QStringList{SearchFilterModel::kAllMembers[memberIndex]};
    }
  }

  applyCurrentFilters();
}

void SearchPage::onSortModeChanged(int index) {
  Q_UNUSED(index);
  applyCurrentFilters();
}

void SearchPage::applyCurrentFilters() {
  // Собираем текущие параметры фильтрации
  const QString searchText = searchEdit_ ? searchEdit_->text() : QString();

  const auto sortMode = static_cast<SearchFilterModel::SortMode>(
      sortCombo_->currentData().toInt());

  // Применяем фильтры через модель
  auto filteredResults =
      filterModel_.applyFilters(searchText, selectedMembers_, sortMode);

  // Обновляем отображение
  resultsList_->setItems(filteredResults);
}
