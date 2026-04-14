#include "TextMarkupWidget.hpp"
#include "FlowLayout.hpp"
#include <QFontMetrics>
#include <QRegularExpression>
#include <QVBoxLayout>

/**
 * @brief Конструктор класса TextMarkupWidget.
 * @param parent Указатель на родительский виджет.
 */
TextMarkupWidget::TextMarkupWidget(QWidget *parent)
    : QWidget(parent), m_wordColor("#000000"), m_labelColor("#808080"),
      m_wordBackgroundColor("#FFFACD"), m_highlightedRole("") {

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  m_container = new QWidget();
  m_container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  m_flowLayout = new FlowLayout(m_container, 5, 5, 5);
  m_container->setLayout(m_flowLayout);

  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidget(m_container);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_scrollArea->setStyleSheet(
      "QScrollArea { border: none; background-color: transparent; }"
      "QScrollArea > QWidget > QWidget { background-color: transparent; }");

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(m_scrollArea);
}

/**
 * @brief Устанавливает текст и карту соответствия слов их ролям.
 * @param text Текст для отображения.
 * @param members Карта соответствия: слово -> роль в предложении.
 */
void TextMarkupWidget::setMarkupText(const QString &text,
                                     std::vector<SentenceResult> &results) {
  m_text = text;
  m_results = results;
  rebuild();
}

/**
 * @brief Устанавливает цвет текста слов.
 * @param color Цвет в формате CSS (например, "#000000").
 */
void TextMarkupWidget::setWordColor(const QString &color) {
  if (m_wordColor != color) {
    m_wordColor = color;
    rebuild();
  }
}

/**
 * @brief Устанавливает цвет подписей (ролей предложения).
 * @param color Цвет в формате CSS (например, "#808080").
 */
void TextMarkupWidget::setLabelColor(const QString &color) {
  if (m_labelColor != color) {
    m_labelColor = color;
    rebuild();
  }
}

/**
 * @brief Возвращает текущий цвет слов.
 * @return Цвет слов в формате CSS.
 */
QString TextMarkupWidget::wordColor() const { return m_wordColor; }

/**
 * @brief Возвращает текущий цвет подписей.
 * @return Цвет подписей в формате CSS.
 */
QString TextMarkupWidget::labelColor() const { return m_labelColor; }

/**
 * @brief Перестраивает отображение текста.
 *
 * Очищает текущее содержимое и заново создает все элементы
 * на основе текущих значений m_text и m_members.
 */
void TextMarkupWidget::rebuild() {
  // 1. Очищаем текущий layout
  QLayoutItem *child;
  while ((child = m_flowLayout->takeAt(0)) != nullptr) {
    delete child->widget();
    delete child;
  }

  if (m_results.empty()) {
    return;
  }

  static const QMap<QString, QString> roleMap = {
      {"подлежащее", "подл."},     {"сказуемое", "сказ."},
      {"обстоятельство", "обст."}, {"определение", "опр."},
      {"дополнение", "доп."},      {"другое", "др."}};

  QFont wordFont("", 11);
  QFontMetrics wordFm(wordFont);
  QFont labelFont("", 7);
  QFontMetrics labelFm(labelFont);
  int containerHeight = wordFm.height() + labelFm.height() + 6;

  for (const auto &sentence : m_results) {
    const QString fullText = QString::fromStdString(sentence.text);

    // Используем итератор
    auto entityIt = sentence.entities.begin();
    auto entityEnd = sentence.entities.end();

    int textPos = 0;

    while (textPos < fullText.length()) {
      QChar ch = fullText.at(textPos);

      if (ch.isSpace()) {
        textPos++;
        continue;
      }

      // Проверяем, указывает ли итератор на сущность, которая начинается здесь
      if (entityIt != entityEnd &&
          textPos == static_cast<int>(entityIt->start)) {
        // --- ВЫВОД СЛОВА ---
        // const Entity &entity = *entityIt;

        QString wordText = QString::fromStdString(
            entityIt->text
                .c_str()); // fullText.mid(textPos, static_cast<int>(entity.end
                           // - entity.start + 1));
        QString fullRole = QString::fromStdString(entityIt->type_ru);
        QString shortRole = roleMap.value(fullRole, fullRole);

        // Переходим за конец текущего слова
        textPos = static_cast<int>(entityIt->end) + 1;

        if (static_cast<int>(entityIt->end) + 1 == fullText.length() - 1) {
          QChar nextChar = fullText.at(fullText.length() - 1);
          wordText += nextChar;
          textPos++;
        }

        QWidget *container = new QWidget();
        container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        container->setFixedHeight(containerHeight);

        QVBoxLayout *blockLayout = new QVBoxLayout(container);
        blockLayout->setSpacing(0);
        blockLayout->setContentsMargins(2, 0, 2, 0);

        QLabel *memberLabel = new QLabel(shortRole);
        memberLabel->setAlignment(Qt::AlignCenter);
        memberLabel->setStyleSheet(
            QString("font-size: 7pt; color: %1;").arg(m_labelColor));

        QLabel *wordLabel = new QLabel(wordText);
        wordLabel->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
        wordLabel->setStyleSheet(
            QString("font-size: 11pt; color: %1;").arg(m_wordColor));
        wordLabel->setToolTip(fullRole);

        blockLayout->addWidget(memberLabel);
        blockLayout->addWidget(wordLabel);

        m_flowLayout->addWidget(container);

        // Переходим к следующей сущности в списке
        ++entityIt;
      } else {
        // --- ВЫВОД ЗНАКА ПРЕПИНАНИЯ ---
        QWidget *container = new QWidget();
        // int punctWidth = wordFm.horizontalAdvance(ch);

        container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        container->setFixedHeight(containerHeight);
        // container->setFixedWidth(punctWidth);

        QVBoxLayout *blockLayout = new QVBoxLayout(container);
        blockLayout->setSpacing(0);
        blockLayout->setContentsMargins(0, 0, 2, 0);

        QLabel *memberLabel = new QLabel("");

        QLabel *wordLabel = new QLabel(ch);
        wordLabel->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
        wordLabel->setStyleSheet(
            QString("font-size: 11pt; color: %1;").arg(m_wordColor));

        blockLayout->addWidget(memberLabel);
        blockLayout->addWidget(wordLabel);

        m_flowLayout->addWidget(container);

        textPos++;
      }
    }
  }

  update();
  m_scrollArea->widget()->update();
}

// Реализация сеттера
/*void TextMarkupWidget::setWordBackgroundColor(const QString &color) {
    if (m_wordBackgroundColor != color) {
        m_wordBackgroundColor = color;
        updateHighlighting(); // Перестраиваем виджет при изменении цвета
    }
}*/

// Реализация геттера
QString TextMarkupWidget::wordBackgroundColor() const {
  return m_wordBackgroundColor;
}

void TextMarkupWidget::setHighlightedRole(const QString &role) {
  if (m_highlightedRole != role) {
    m_highlightedRole = role;
    updateHighlighting(); // Перестраиваем интерфейс при изменении роли
  }
}

QString TextMarkupWidget::highlightedRole() const { return m_highlightedRole; }

void TextMarkupWidget::updateHighlighting() {
  if (m_text.isEmpty()) {
    return;
  }

  // Проходим по всем контейнерам в layout'е
  for (int i = 0; i < m_flowLayout->count(); ++i) {
    QWidget *container = m_flowLayout->itemAt(i)->widget();
    if (!container)
      continue;

    // Получаем вертикальный макет контейнера (QVBoxLayout)
    QVBoxLayout *vLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!vLayout || vLayout->count() < 2)
      continue; // Проверяем, что макет корректен

    // 1. Находим QLabel с ролью (подпись сверху)
    QLabel *memberLabel = qobject_cast<QLabel *>(vLayout->itemAt(0)->widget());
    if (!memberLabel)
      continue;

    // 2. Находим QLabel со словом (текст снизу)
    // Теперь это просто второй элемент в QVBoxLayout, а не вложенный поиск.
    QLabel *wordLabel = qobject_cast<QLabel *>(vLayout->itemAt(1)->widget());
    if (!wordLabel)
      continue;

    if (memberLabel->text().isEmpty()) {
      continue; // Переходим к следующему виджету
    }

    // Проверяем, совпадает ли роль с той, которую нужно выделить
    if (memberLabel->text() == m_highlightedRole) {
      // Применяем выделение к слову
      wordLabel->setStyleSheet(
          QString("font-size: 11pt; color: %1; font-weight: normal; "
                  "background-color: %2; padding: 1px; border-radius: 2px;")
              .arg(m_wordColor)
              .arg(m_wordBackgroundColor));
    } else {
      // Убираем выделение (сбрасываем стиль на базовый)
      wordLabel->setStyleSheet(
          QString("font-size: 11pt; color: %1; font-weight: normal; "
                  "background-color: transparent;")
              .arg(m_wordColor));
    }
  }
}
