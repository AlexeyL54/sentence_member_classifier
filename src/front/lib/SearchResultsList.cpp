#include "SearchResultsList.hpp"

#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace {
constexpr int kCardMargins = 16;
constexpr int kCardSpacing = 8;
constexpr int kListMargins = 20;
constexpr int kListSpacing = 12;
constexpr int kSnippetMaxLength = 90;
constexpr qreal kFontSizeMultiplier = 1.05;
} // namespace

SearchResultsList::SearchResultsList(QWidget *parent) : QWidget(parent) {
  scrollArea_ = new QScrollArea(this);
  scrollArea_->setWidgetResizable(true);
  scrollArea_->setFrameShape(QFrame::NoFrame);
  scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  container_ = new QWidget(scrollArea_);
  layout_ = new QVBoxLayout(container_);
  layout_->setContentsMargins(kListMargins, kListMargins, kListMargins,
                              kListMargins);
  layout_->setSpacing(kListSpacing);
  // Убираем выравнивание по верху, чтобы карточки могли растягиваться
  // layout_->setAlignment(Qt::AlignTop); - УДАЛЯЕМ ЭТУ СТРОКУ

  scrollArea_->setWidget(container_);

  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->addWidget(scrollArea_);
}

void SearchResultsList::setItems(const std::vector<SearchItem> &items) {
  clearLayout();

  for (const auto &item : items) {
    QFrame *card = createCard(item);
    // Разрешаем карточке растягиваться по вертикали
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout_->addWidget(card);
  }

  // Растягивающий элемент в конце списка
  layout_->addStretch(1);
}

void SearchResultsList::clearLayout() {
  if (!layout_)
    return;

  // Блокируем обновление для производительности
  scrollArea_->setUpdatesEnabled(false);

  QLayoutItem *item;
  while ((item = layout_->takeAt(0)) != nullptr) {
    if (QWidget *widget = item->widget()) {
      widget->deleteLater();
    }
    delete item;
  }

  scrollArea_->setUpdatesEnabled(true);
}

QFrame *SearchResultsList::createCard(const SearchItem &item) {
  auto *card = new QFrame(container_);
  card->setFrameShape(QFrame::Box);
  card->setLineWidth(1);
  card->setStyleSheet(
      "QFrame { background: palette(base); border-radius: 4px; }");

  // Разрешаем карточке растягиваться по вертикали
  card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  auto *cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(kCardMargins, kCardMargins, kCardMargins,
                                 kCardMargins);
  cardLayout->setSpacing(kCardSpacing);
  // Убираем растягивание внутри карточки, чтобы она сжималась под содержимое
  cardLayout->setAlignment(Qt::AlignTop);

  // 1. Слово (жирным шрифтом)
  auto *wordLabel = new QLabel(QString::fromStdString(item.text), card);
  QFont wordFont = wordLabel->font();
  wordFont.setBold(true);
  wordFont.setPointSizeF(wordFont.pointSizeF() * kFontSizeMultiplier);
  wordLabel->setFont(wordFont);
  wordLabel->setWordWrap(true);
  cardLayout->addWidget(wordLabel);

  // 2. Тип члена предложения
  auto *memberLabel = new QLabel(QStringLiteral("Член предложения: %1")
                                     .arg(QString::fromStdString(item.type)),
                                 card);
  memberLabel->setWordWrap(true);
  memberLabel->setStyleSheet("color: gray;");
  cardLayout->addWidget(memberLabel);

  // 3. Количество вхождений
  auto *countLabel = new QLabel(
      QStringLiteral("Количество вхождений: %1").arg(item.amount), card);
  countLabel->setWordWrap(true);
  cardLayout->addWidget(countLabel);

  // 4. Контексты использования
  QWidget *sentencesSection = createSentencesSection(item.sentences, card);
  cardLayout->addWidget(sentencesSection);

  return card;
}

QWidget *SearchResultsList::createSentencesSection(
    const std::vector<std::pair<int, std::string>> &sentences,
    QWidget *parent) {
  auto *container = new QWidget(parent);
  auto *layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 4, 0, 0);
  layout->setSpacing(4);
  layout->setAlignment(Qt::AlignTop);

  if (sentences.empty()) {
    auto *emptyLabel =
        new QLabel(QStringLiteral("Предложения: нет"), container);
    emptyLabel->setWordWrap(true);
    emptyLabel->setStyleSheet("color: gray; font-style: italic;");
    layout->addWidget(emptyLabel);
    return container;
  }

  // Заголовок секции
  auto *headerLabel =
      new QLabel(QStringLiteral("Контексты использования:"), container);
  headerLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
  layout->addWidget(headerLabel);

  // Карточки контекстов
  for (const auto &ctx : sentences) {
    QString snippet = QString::fromStdString(ctx.second).trimmed();

    if (snippet.isEmpty())
      continue;

    // Обрезаем длинные фрагменты
    if (snippet.size() > kSnippetMaxLength) {
      snippet = snippet.left(kSnippetMaxLength) + QStringLiteral("...");
    }

    auto *sentenceLabel = new QLabel(
        QStringLiteral("• Предложение №%1: %2").arg(ctx.first).arg(snippet),
        container);
    sentenceLabel->setWordWrap(true);
    sentenceLabel->setStyleSheet("background: palette(alternate-base); "
                                 "padding: 4px; border-radius: 2px;");
    layout->addWidget(sentenceLabel);
  }

  return container;
}
