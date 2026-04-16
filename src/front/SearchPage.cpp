#include "SearchPage.hpp"

#include <algorithm>
#include <limits>
#include <string>

#include <QAbstractItemView>
#include <QDateTime>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QMouseEvent>
#include <QPushButton>
#include <QSizePolicy>
#include <QStandardItem>
#include <QStringList>
#include <QVBoxLayout>

/**
 * @brief Контейнер прокручиваемого списка результатов поиска.
 *
 * Внутри вызова setItems() динамически создаются карточки с результатами.
 * @param parent Родительский виджет.
 */
SearchResultsList::SearchResultsList(QWidget *parent) : QWidget(parent) {
  QScrollArea *scroll = new QScrollArea(this);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);

  container_ = new QWidget(scroll);
  layout_ = new QVBoxLayout(container_);
  layout_->setContentsMargins(20, 20, 20, 20);
  layout_->setSpacing(12);

  scroll->setWidget(container_);

  QVBoxLayout *root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->addWidget(scroll);
}

/**
 * @brief Удаляет ранее созданные карточки из layout'а
 */
void SearchResultsList::clearLayout() {
  if (!layout_)
    return;

  while (QLayoutItem *item = layout_->takeAt(0)) {
    if (QWidget *w = item->widget())
      w->deleteLater();
    delete item;
  }
}

/**
 * @brief Перерисовывает список карточек по набору элементов.
 * @param items Набор элементов для отображения в виде карточек.
 */
void SearchResultsList::setItems(const std::vector<SearchItem> &items) {
  clearLayout();

  for (const auto &it : items) {
    QFrame *card = new QFrame(container_);
    card->setFrameShape(QFrame::Box);
    card->setLineWidth(1);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(8);

    // 1) Слово (член предложения)
    QLabel *wordLabel = new QLabel(QString::fromStdString(it.text), card);
    QFont f = wordLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.05);
    wordLabel->setFont(f);
    wordLabel->setWordWrap(true);

    // 2) Что за член предложения
    QLabel *memberLabel = new QLabel(QStringLiteral("Член предложения: %1")
                                         .arg(QString::fromStdString(it.type)),
                                     card);
    memberLabel->setWordWrap(true);

    // 3) Встречаемость (как раньше счётчик в данных)
    QLabel *countLabel = new QLabel(
        QStringLiteral("Количество вхождений: %1").arg(it.amount), card);
    countLabel->setWordWrap(true);

    cardLayout->addWidget(wordLabel);
    cardLayout->addWidget(memberLabel);
    cardLayout->addWidget(countLabel);

    // 4) Предложения-контексты (номер предложения в тексте + превью)
    if (it.sentences.empty()) {
      QLabel *emptyLabel = new QLabel(QStringLiteral("Предложения: нет"), card);
      emptyLabel->setWordWrap(true);
      cardLayout->addWidget(emptyLabel);
    } else {
      for (const auto &ctx : it.sentences) {
        QString snippet = QString::fromStdString(ctx.second).trimmed();
        if (!snippet.isEmpty()) {
          if (snippet.size() > 90)
            snippet = snippet.left(90) + QStringLiteral("...");
          QString sentenceLine = QStringLiteral("В предложении №%1: %2")
                                     .arg(ctx.first)
                                     .arg(snippet);
          QLabel *sentenceLabel = new QLabel(sentenceLine, card);
          sentenceLabel->setWordWrap(true);
          cardLayout->addWidget(sentenceLabel);
        }
      }
    }

    layout_->addWidget(card);
  }

  layout_->addStretch(1);
}

/**
 * @brief Страница поиска: кнопка «Назад», поле ввода, фильтры и список
 * результатов.
 * @param items Набор данных, который отображается и фильтруется на странице.
 * @param parent Родительский виджет.
 */
SearchPage::SearchPage(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *root = new QVBoxLayout(this);
  // Увеличиваем отступы, чтобы элементы не были прижаты к краям.
  root->setContentsMargins(24, 20, 24, 24);
  root->setSpacing(14);
  const int controlHeight = 38; // Единая высота элементов управления
  const int searchWidth = 640;  // Общая ширина поискового блока

  QPushButton *backBtn = new QPushButton(QStringLiteral("Назад"), this);
  backBtn->setCursor(Qt::PointingHandCursor);
  connect(backBtn, &QPushButton::clicked, this, &SearchPage::backRequested);
  backBtn->setMinimumHeight(controlHeight);

  // Верхняя строка только для кнопки "Назад"
  QHBoxLayout *backRow = new QHBoxLayout();
  backRow->setContentsMargins(0, 0, 0, 0);
  backRow->addWidget(backBtn, 0, Qt::AlignLeft | Qt::AlignVCenter);
  backRow->addStretch(1);
  root->addLayout(backRow, 0);

  searchEdit_ = new QLineEdit(this);
  searchEdit_->setPlaceholderText(
      QStringLiteral("Введите слово для поиска..."));
  searchEdit_->setFixedWidth(searchWidth);
  searchEdit_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  searchEdit_->setMinimumHeight(controlHeight);

  // Члены предложения: выпадающий список с чекбоксами
  memberFilterCombo_ = new QComboBox(this);
  memberFilterCombo_->setEditable(true);
  memberFilterCombo_->lineEdit()->setReadOnly(true);
  memberFilterCombo_->setInsertPolicy(QComboBox::NoInsert);
  memberFilterCombo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  memberFilterCombo_->setMinimumHeight(controlHeight);
  memberFilterCombo_->setFixedWidth((searchWidth - 12) / 2);

  // Список вариантов
  const QVector<QString> memberOptions = {
      QStringLiteral("подлежащее"),     QStringLiteral("сказуемое"),
      QStringLiteral("дополнение"),     QStringLiteral("определение"),
      QStringLiteral("обстоятельство"),
  };

  memberModel_ = new QStandardItemModel(memberFilterCombo_);

  // Служебный пункт: быстро отметить/снять все категории.
  QStandardItem *selectAllItem =
      new QStandardItem(QStringLiteral("Выбрать всё"));
  selectAllItem->setFlags(selectAllItem->flags() | Qt::ItemIsUserCheckable);
  selectAllItem->setData(Qt::Checked, Qt::CheckStateRole);
  memberModel_->appendRow(selectAllItem);

  for (const auto &m : memberOptions) {
    QStandardItem *item = new QStandardItem(m);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setData(Qt::Checked, Qt::CheckStateRole);
    memberModel_->appendRow(item);
  }
  memberFilterCombo_->setModel(memberModel_);

  if (auto *view = qobject_cast<QListView *>(memberFilterCombo_->view())) {
    view->setSelectionMode(QAbstractItemView::NoSelection);
    // Перехватываем клики по popup, чтобы галочки ставились без закрытия
    // списка.
    view->viewport()->installEventFilter(this);
    view->installEventFilter(this);
  }
  // Открытие/закрытие — только с lineEdit (не вешать фильтр на весь QComboBox:
  // иначе двойная обработка и popup не открывается стабильно).
  memberFilterCombo_->lineEdit()->installEventFilter(this);

  // Сортировка: выпадающий список (1 вариант)
  sortCombo_ = new QComboBox(this);
  sortCombo_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  sortCombo_->setMinimumHeight(controlHeight);
  sortCombo_->setFixedWidth((searchWidth - 12) / 2);

  // Сортировка в обе стороны для каждой категории
  sortCombo_->addItem(QStringLiteral("По алфавиту: А→Я"), 0);
  sortCombo_->addItem(QStringLiteral("По алфавиту: Я→А"), 1);
  sortCombo_->addItem(QStringLiteral("По числу появления: по убыванию"), 2);
  sortCombo_->addItem(QStringLiteral("По числу появления: по возрастанию"), 3);
  // sortCombo_->addItem(QStringLiteral("По номеру предложения: по
  // возрастанию"),
  //                  4);
  // sortCombo_->addItem(QStringLiteral("По номеру предложения: по убыванию"),
  // 5);
  connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &SearchPage::onSortModeChanged);

  // Центральный контейнер:
  // поле поиска и оба фильтра имеют общую ширину и ровные боковые границы.
  QWidget *contentWrap = new QWidget(this);
  contentWrap->setFixedWidth(searchWidth);
  QVBoxLayout *contentColumn = new QVBoxLayout(contentWrap);
  contentColumn->setContentsMargins(0, 0, 0, 0);
  contentColumn->setSpacing(12);

  contentColumn->addWidget(searchEdit_, 0, Qt::AlignHCenter);

  // --------- Фильтры (в одной строке, на одной высоте) ----------
  QHBoxLayout *filtersLayout = new QHBoxLayout();
  filtersLayout->setContentsMargins(0, 0, 0, 0);
  filtersLayout->setSpacing(12);
  filtersLayout->addWidget(memberFilterCombo_, 0,
                           Qt::AlignLeft | Qt::AlignVCenter);
  filtersLayout->addWidget(sortCombo_, 0, Qt::AlignRight | Qt::AlignVCenter);
  contentColumn->addLayout(filtersLayout, 0);

  // --------- Список результатов ----------
  resultsList_ = new SearchResultsList(this);
  resultsList_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  contentColumn->addWidget(resultsList_, 1);

  root->addWidget(contentWrap, 1, Qt::AlignHCenter);

  // --------- Сигналы ----------
  connect(searchEdit_, &QLineEdit::textChanged, this,
          &SearchPage::onSearchTextChanged);

  // --------- Инициализация состояния ----------
  sortModeIndex_ = sortCombo_->currentIndex();
  updateSelectedMembers();
  updateMemberComboSummary();
  applyCurrentFilters();
}

void SearchPage::setSearchItems(std::vector<SearchItem> &items) {
  allItems_ = items;
  pendingMemberFilterApply_ = false;
  applyCurrentFilters();
}

/**
 * @brief Перехватывает клики для мультимножественного выбора в выпадающем
 * списке.
 * @param watched Объект, на котором произошло событие.
 * @param event Событие Qt.
 * @return true, если событие обработано в фильтре и не должно идти дальше;
 * иначе false.
 */
bool SearchPage::eventFilter(QObject *watched, QEvent *event) {
  if (!memberFilterCombo_ || !memberModel_)
    return QWidget::eventFilter(watched, event);

  // Пока открыт popup фильтра — не перерисовывать тяжёлый список карточек под
  // ним (иначе главный поток занят и выпадашка «рвётся»).
  if (memberFilterCombo_->view() && watched == memberFilterCombo_->view()) {
    if (event->type() == QEvent::Show) {
      if (resultsList_)
        resultsList_->setUpdatesEnabled(false);
      return false;
    }
    if (event->type() == QEvent::Hide) {
      if (resultsList_)
        resultsList_->setUpdatesEnabled(true);
      if (pendingMemberFilterApply_) {
        pendingMemberFilterApply_ = false;
        applyCurrentFilters();
      }
      if (memberSkipBlockOnNextHide_) {
        memberSkipBlockOnNextHide_ = false;
      } else {
        memberBlockShowPopupUntilMs_ =
            QDateTime::currentMSecsSinceEpoch() + kMemberPopupBlockMs;
      }
      return false;
    }
  }

  // Клик по строке с текстом: открыть/закрыть список (только lineEdit).
  if (watched == memberFilterCombo_->lineEdit() &&
      event->type() == QEvent::MouseButtonPress) {
    auto *me = static_cast<QMouseEvent *>(event);
    if (me->button() != Qt::LeftButton)
      return QWidget::eventFilter(watched, event);
    if (QAbstractItemView *v = memberFilterCombo_->view()) {
      if (v->isVisible()) {
        memberSkipBlockOnNextHide_ = true;
        memberFilterCombo_->hidePopup();
        return true;
      }
      if (QDateTime::currentMSecsSinceEpoch() < memberBlockShowPopupUntilMs_)
        return true;
      memberFilterCombo_->showPopup();
      return true;
    }
  }

  // Клик по элементам popup: переключаем галку и не закрываем список.
  if (memberFilterCombo_->view() &&
      watched == memberFilterCombo_->view()->viewport() &&
      event->type() == QEvent::MouseButtonRelease) {
    auto *me = static_cast<QMouseEvent *>(event);
    QModelIndex idx = memberFilterCombo_->view()->indexAt(me->pos());
    if (!idx.isValid())
      return true;

    QStandardItem *item = memberModel_->itemFromIndex(idx);
    if (!item)
      return true;

    const int row = idx.row();
    QStandardItem *selectAll = memberModel_->item(0);

    if (row == 0) {
      // Клик по «Выбрать всё» переключает все чекбоксы сразу.
      const Qt::CheckState targetState =
          (item->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
      for (int i = 0; i < memberModel_->rowCount(); ++i) {
        if (QStandardItem *each = memberModel_->item(i))
          each->setCheckState(targetState);
      }
    } else {
      item->setCheckState(item->checkState() == Qt::Checked ? Qt::Unchecked
                                                            : Qt::Checked);

      // Автосинхронизация «Выбрать всё» с фактическим состоянием остальных
      // пунктов.
      bool allChecked = true;
      for (int i = 1; i < memberModel_->rowCount(); ++i) {
        QStandardItem *each = memberModel_->item(i);
        if (!each || each->checkState() != Qt::Checked) {
          allChecked = false;
          break;
        }
      }
      if (selectAll)
        selectAll->setCheckState(allChecked ? Qt::Checked : Qt::Unchecked);
    }

    updateSelectedMembers();
    updateMemberComboSummary();
    // Не вызываем applyCurrentFilters здесь: setItems пересоздаёт виджеты и
    // мешает плавности popup. Пересборка — в QEvent::Hide у view (см. выше).
    pendingMemberFilterApply_ = true;
    return true;
  }

  return QWidget::eventFilter(watched, event);
}

/**
 * @brief Обновляет selectedMembers_ по текущему состоянию check-боксов.
 */
void SearchPage::updateSelectedMembers() {
  selectedMembers_.clear();
  if (!memberModel_)
    return;

  for (int row = 1; row < memberModel_->rowCount(); ++row) {
    QStandardItem *item = memberModel_->item(row);
    if (!item)
      continue;
    if (item->checkState() == Qt::Checked)
      selectedMembers_.push_back(item->text());
  }
}

/**
 * @brief Обновляет текст в поле memberFilterCombo_ в зависимости от выбранных
 * чеков.
 */
void SearchPage::updateMemberComboSummary() {
  if (!memberModel_ || !memberFilterCombo_)
    return;

  const int total = std::max(0, memberModel_->rowCount() - 1);
  if (selectedMembers_.size() == static_cast<size_t>(total) && total > 0) {
    memberFilterCombo_->lineEdit()->setText(
        QStringLiteral("Все члены предложения"));
    return;
  }

  if (selectedMembers_.empty()) {
    memberFilterCombo_->lineEdit()->setText(
        QStringLiteral("Нет выбранных членов"));
    return;
  }

  QStringList parts;
  parts.reserve(selectedMembers_.size());
  for (const auto &m : selectedMembers_)
    parts.push_back(m);
  memberFilterCombo_->lineEdit()->setText(parts.join(QStringLiteral(", ")));
}

/**
 * @brief Применяет фильтры (по тексту/членам/сортировке) и обновляет список.
 */
void SearchPage::applyCurrentFilters() {
  filterAndRender(searchEdit_ ? searchEdit_->text() : QString());
}

/**
 * @brief Фильтрует элементы по тексту поиска, члену предложения и сортирует их.
 * @param text Текущее значение из поля поиска.
 */
void SearchPage::filterAndRender(const QString &text) {
  const QString needle = text.trimmed();

  // Считаем фильтр по членам актуальным (на случай изменений вне
  // pressed-коннекта).
  updateSelectedMembers();

  std::vector<SearchItem> filtered;
  filtered.reserve(allItems_.size());

  for (const auto &it : allItems_) {
    const QString qtext = QString::fromStdString(it.text);
    if (!needle.isEmpty() && !qtext.contains(needle, Qt::CaseInsensitive))
      continue;

    // Если пользователь снял все галки — показать пустой список.
    if (selectedMembers_.empty())
      continue;

    const QString qtype = QString::fromStdString(it.type);
    if (!selectedMembers_.contains(qtype))
      continue;

    filtered.push_back(it);
  }

  auto wordKey = [](const SearchItem &it) {
    return QString::fromStdString(it.text).toLower();
  };

  // Режимы 4–5: по минимальному номеру предложения среди контекстов.
  auto minSentenceNumber = [](const SearchItem &it) {
    if (it.sentences.empty())
      return std::numeric_limits<int>::max();
    int m = it.sentences.front().first;
    for (const auto &c : it.sentences)
      m = std::min(m, c.first);
    return m;
  };

  // Применяем сортировку
  switch (sortModeIndex_) {
  case 0: // По алфавиту: А->Я
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                return wordKey(a) < wordKey(b);
              });
    break;
  case 1: // По алфавиту: Я->А
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                return wordKey(a) > wordKey(b);
              });
    break;
  case 2: // По встречаемости: по убыванию
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                if (a.amount != b.amount)
                  return a.amount > b.amount;
                return wordKey(a) < wordKey(b);
              });
    break;
  case 3: // По встречаемости: по возрастанию
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                if (a.amount != b.amount)
                  return a.amount < b.amount;
                return wordKey(a) < wordKey(b);
              });
    break;
  /*case 4:
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                if (minSentenceNumber(a) != minSentenceNumber(b))
                  return minSentenceNumber(a) < minSentenceNumber(b);
                return wordKey(a) < wordKey(b);
              });
    break;
  case 5:
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                if (minSentenceNumber(a) != minSentenceNumber(b))
                  return minSentenceNumber(a) > minSentenceNumber(b);
                return wordKey(a) < wordKey(b);
              });
    break;*/
  default:
    // На случай некорректного индекса: алфавит по возрастанию.
    std::sort(filtered.begin(), filtered.end(),
              [&](const SearchItem &a, const SearchItem &b) {
                return wordKey(a) < wordKey(b);
              });
    break;
  }

  resultsList_->setItems(filtered);
}

/**
 * @brief Реагирует на ввод в поле поиска.
 *
 * Фильтрация применяется сразу (как «живой поиск»).
 * @param text Текущий текст из поля поиска.
 */
void SearchPage::onSearchTextChanged(const QString &text) {
  filterAndRender(text);
}

/**
 * @brief Реагирует на изменение выбранных членов предложения.
 */
void SearchPage::onMemberFilterChanged() {
  updateSelectedMembers();
  updateMemberComboSummary();
  applyCurrentFilters();
}

/**
 * @brief Реагирует на изменение выбранного режима сортировки.
 * @param index Индекс выбранного режима в выпадающем списке сортировки.
 */
void SearchPage::onSortModeChanged(int index) {
  sortModeIndex_ = index;
  applyCurrentFilters();
}
