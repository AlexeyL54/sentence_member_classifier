#include "SearchResultsList.hpp"

#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <utility>

namespace {
constexpr int kCardMargins = 16;      // Отступы внутри карточки
constexpr int kCardSpacing = 8;       // Расстояние между элементами в карточке
constexpr int kListMargins = 20;      // Отступы списка от краёв
constexpr int kListSpacing = 12;      // Расстояние между карточками
constexpr int kSnippetMaxLength = 90; // Максимальная длина контекста
constexpr qreal kFontSizeMultiplier = 1.05; // Множитель шрифта
} // namespace

/**
 * @brief Конструктор виджета списка результатов.
 * @param parent Родительский виджет (по умолчанию nullptr).
 */
SearchResultsList::SearchResultsList(QWidget *parent) : QWidget(parent) {
  setupUi();
}

/**
 * @brief Настраивает основную компоновку виджета.
 *
 * Создаёт область прокрутки, контейнер для карточек и корневой layout.
 * Устанавливает политики отображения и размеров.
 * Добавляет stretch в конец layout_ для прижатия карточек к верху.
 */
void SearchResultsList::setupUi() {
  scrollArea_ = new QScrollArea(this);
  scrollArea_->setWidgetResizable(true);
  scrollArea_->setFrameShape(QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  container_ = new QWidget(scrollArea_);
  container_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
  layout_ = new QVBoxLayout(container_);
  layout_->setContentsMargins(kListMargins, kListMargins, kListMargins,
                              kListMargins);
  layout_->setSpacing(kListSpacing);
  layout_->setAlignment(Qt::AlignTop);

  scrollArea_->setWidget(container_);

  QVBoxLayout *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->addWidget(scrollArea_);
}

/**
 * @brief Обновляет содержимое списка новым набором элементов.
 *
 * Полностью заменяет текущее содержимое. Старые карточки уничтожаются
 * через deleteLater() для безопасной работы с событиями Qt.
 *
 * @param items Новый набор элементов для отображения.
 */
void SearchResultsList::setItems(const std::vector<SearchItem> &items) {
  // Блокируем обновления для плавной перерисовки
  setUpdatesEnabled(false);

  while (layout_->count() > 0) {
    QLayoutItem *item = layout_->takeAt(0);
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }
    delete item;
  }

  // Добавляем новые карточки
  for (const SearchItem &item : items) {
    QFrame *card = createCard(item);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    card->setMinimumHeight(0);
    layout_->addWidget(card);
  }
  container_->adjustSize();
  scrollArea_->updateGeometry();

  setUpdatesEnabled(true);
}

/**
 * @brief Удаляет все карточки из layout'а.
 *
 * Безопасно очищает layout, планируя удаление виджетов
 * через очередь событий Qt.
 * Перед очисткой отключает обновления scrollArea для повышения
 * производительности.
 */
void SearchResultsList::clearLayout() {
  if (!layout_)
    return;

  scrollArea_->setUpdatesEnabled(false);

  // Удаляем все виджеты
  while (layout_->count() > 0) {
    QLayoutItem *item = layout_->takeAt(0);
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }
    delete item;
  }

  scrollArea_->setUpdatesEnabled(true);
}

/**
 * @brief Настраивает внешний вид карточки.
 *
 * Устанавливает рамку, толщину границы и стиль с закруглёнными углами.
 * Использует системную цветовую схему palette(base).
 *
 * @param card Карточка для настройки.
 */
void SearchResultsList::setupCardStyle(QFrame *card) {
  card->setFrameShape(QFrame::Box);
  card->setLineWidth(1);
  card->setStyleSheet(
      "QFrame { background: palette(base); border-radius: 4px; }");
}

/**
 * @brief Создаёт метку с основным словом.
 *
 * Устанавливает жирное начертание и увеличенный размер шрифта.
 * Включает перенос слов для длинных терминов.
 *
 * @param text Текст слова.
 * @param parent Родительский виджет.
 * @return Указатель на созданную метку.
 */
QLabel *SearchResultsList::createWordLabel(const QString &text,
                                           QWidget *parent) {
  QLabel *label = new QLabel(text, parent);
  QFont font = label->font();
  font.setBold(true);
  font.setPointSizeF(font.pointSizeF() * kFontSizeMultiplier);
  label->setFont(font);
  label->setWordWrap(true);
  return label;
}

/**
 * @brief Создаёт метку с типом члена предложения.
 *
 * Форматирует строку с префиксом "Член предложения:".
 * Включает перенос слов для длинных описаний типов.
 *
 * @param type Тип члена предложения.
 * @param parent Родительский виджет.
 * @return Указатель на созданную метку.
 */
QLabel *SearchResultsList::createMemberTypeLabel(const QString &type,
                                                 QWidget *parent) {
  QLabel *label =
      new QLabel(QStringLiteral("Член предложения: %1").arg(type), parent);
  label->setWordWrap(true);
  return label;
}

/**
 * @brief Создаёт метку с количеством вхождений.
 *
 * Форматирует строку с префиксом "Количество вхождений:".
 *
 * @param amount Количество вхождений.
 * @param parent Родительский виджет.
 * @return Указатель на созданную метку.
 */
QLabel *SearchResultsList::createCountLabel(int amount, QWidget *parent) {
  QLabel *label = new QLabel(
      QStringLiteral("Количество вхождений: %1").arg(amount), parent);
  label->setWordWrap(true);
  return label;
}

/**
 * @brief Обрезает текст контекста до максимальной длины.
 *
 * Удаляет лишние пробелы в начале и конце строки.
 * Если текст превышает kSnippetMaxLength символов, обрезает его
 * и добавляет многоточие в конец.
 *
 * @param text Исходный текст контекста.
 * @return Обрезанный текст с многоточием при необходимости.
 */
QString SearchResultsList::truncateSnippet(const QString &text) const {
  QString snippet = text.trimmed();
  if (snippet.size() > kSnippetMaxLength) {
    snippet = snippet.left(kSnippetMaxLength) + QStringLiteral("...");
  }
  return snippet;
}

/**
 * @brief Создаёт виджет для одного контекста предложения.
 *
 * Форматирует строку с номером предложения и обрезанным текстом.
 * Добавляет стилизацию с фоном и отступами.
 * Если текст после обрезки пуст, возвращает nullptr.
 *
 * @param sentencePair Пара (номер предложения, текст контекста).
 * @param parent Родительский виджет.
 * @return Указатель на созданную метку или nullptr при пустом тексте.
 */
QLabel *SearchResultsList::createSentenceWidget(
    const std::pair<int, QString> &sentencePair, QWidget *parent) {

  QString snippet = truncateSnippet(sentencePair.second);
  if (snippet.isEmpty())
    return nullptr;

  QLabel *label = new QLabel(QStringLiteral("• Предложение №%1: %2")
                                 .arg(sentencePair.first)
                                 .arg(snippet),
                             parent);
  label->setWordWrap(true);
  label->setStyleSheet("background: palette(alternate-base); "
                       "padding: 4px; border-radius: 2px;");
  return label;
}

/**
 * @brief Создаёт секцию с информацией о контекстах использования.
 *
 * Если список предложений пуст, отображает серую надпись "Предложения: нет".
 * Иначе создаёт заголовок "Контексты использования:" и список всех предложений.
 * Каждое предложение отображается в отдельной стилизованной карточке.
 *
 * @param sentences Вектор пар (номер предложения, текст контекста).
 * @param parent Родительский виджет для создаваемых элементов.
 * @return Указатель на созданный виджет с контекстами.
 */
QWidget *SearchResultsList::createSentencesSection(
    const std::vector<std::pair<int, QString>> &sentences, QWidget *parent) {

  QWidget *container = new QWidget(parent);
  QVBoxLayout *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 0);
  layout->setSpacing(4);
  layout->setAlignment(Qt::AlignTop);
  layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

  if (sentences.empty()) {
    QLabel *emptyLabel =
        new QLabel(QStringLiteral("Предложения: нет"), container);
    emptyLabel->setWordWrap(true);
    emptyLabel->setStyleSheet("color: gray; font-style: italic;");
    layout->addWidget(emptyLabel);
    return container;
  }

  QLabel *headerLabel =
      new QLabel(QStringLiteral("Контексты использования:"), container);
  headerLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
  layout->addWidget(headerLabel);

  for (const auto &ctx : sentences) {
    QLabel *sentenceLabel = createSentenceWidget(ctx, container);
    if (sentenceLabel) {
      layout->addWidget(sentenceLabel);
    }
  }

  return container;
}

/**
 * @brief Создаёт одну карточку для элемента поиска.
 *
 * Карточка содержит:
 * - Основное слово (жирный увеличенный шрифт)
 * - Тип члена предложения
 * - Количество вхождений
 * - Секцию с контекстами использования
 *
 * Все элементы располагаются вертикально с отступами.
 * Карточка имеет стилизованную рамку и фон.
 *
 * @param item Элемент поиска для отображения.
 * @return Указатель на созданный фрейм с карточкой.
 */
QFrame *SearchResultsList::createCard(const SearchItem &item) {
  QFrame *card = new QFrame(container_);
  setupCardStyle(card);
  card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  QVBoxLayout *cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(kCardMargins, kCardMargins, kCardMargins,
                                 kCardMargins);
  cardLayout->setSpacing(kCardSpacing);
  cardLayout->setAlignment(Qt::AlignTop);

  cardLayout->addWidget(createWordLabel(item.text, card));
  cardLayout->addWidget(createMemberTypeLabel(item.type, card));
  cardLayout->addWidget(createCountLabel(item.amount, card));
  cardLayout->addWidget(createSentencesSection(item.sentences, card));

  return card;
}
