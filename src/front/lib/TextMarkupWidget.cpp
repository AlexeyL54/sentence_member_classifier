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
      m_wordBackgroundColor("#FFFACD"), m_highlightedRole("")  {

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
                                     const QMap<QString, QString> &members) {
  m_text = text;
  m_members = members;
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
  // Очищаем layout
  QLayoutItem *child;
  while ((child = m_flowLayout->takeAt(0)) != nullptr) {
    delete child->widget();
    delete child;
  }

  if (m_text.isEmpty()) {
    return;
  }

  // Разбиваем текст на слова с прикрепленными знаками препинания
  QRegularExpression re("(\\p{L}+[.,!?;:()\"'-]*|\\s+)");
  QRegularExpressionMatchIterator i = re.globalMatch(m_text);

  QList<QString> tokens;
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString token = match.captured(0);
    if (!token.trimmed().isEmpty()) {
      tokens.append(token.trimmed());
    }
  }

  // Определяем высоту для контейнеров
  QFont wordFont("", 11);
  QFontMetrics wordFm(wordFont);
  QFont labelFont("", 7);
  QFontMetrics labelFm(labelFont);
  int containerHeight = wordFm.height() + labelFm.height() + 6;

  for (const QString &token : tokens) {
    // Проверяем, содержит ли токен буквы
    QRegularExpression wordRe("\\p{L}+");
    bool hasWord = wordRe.match(token).hasMatch();

    QWidget *container = new QWidget();
    container->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    container->setMinimumHeight(containerHeight);
    container->setMaximumHeight(containerHeight);

    if (hasWord) {
      // Разделяем слово и знаки препинания
      QRegularExpression wordOnlyRe("(\\p{L}+)");
      QRegularExpressionMatch wordMatch = wordOnlyRe.match(token);
      QString word = wordMatch.captured(1);

      // Знаки препинания (все, что после слова)
      QString punctuation = token;
      punctuation.remove(word);

      QVBoxLayout *wordLayout = new QVBoxLayout(container);
      wordLayout->setSpacing(0);
      wordLayout->setContentsMargins(2, 0, 2, 0);

      // Подпись члена предложения
      QString memberText = m_members.value(word, "-");
      QLabel *memberLabel = new QLabel(memberText);
      memberLabel->setAlignment(Qt::AlignCenter);
      memberLabel->setStyleSheet(
          QString("font-size: 7pt; color: %1; font-weight: normal; "
                  "letter-spacing: 0.5px;")
              .arg(m_labelColor));

      // Горизонтальный layout для слова и знаков препинания
      QHBoxLayout *wordWithPunctLayout = new QHBoxLayout();
      wordWithPunctLayout->setSpacing(0);
      wordWithPunctLayout->setContentsMargins(0, 0, 0, 0);

      // Само слово
      QLabel *wordLabel = new QLabel(word);
      wordLabel->setAlignment(Qt::AlignCenter);
      wordLabel->setStyleSheet(
          QString("font-size: 11pt; color: %1; font-weight: normal;")
              .arg(m_wordColor));

      wordWithPunctLayout->addWidget(wordLabel);

      // Знаки препинания (если есть)
      if (!punctuation.isEmpty()) {
        QLabel *punctLabel = new QLabel(punctuation);
        punctLabel->setAlignment(Qt::AlignCenter);
        punctLabel->setStyleSheet(
            QString("font-size: 11pt; color: %1; font-weight: normal; "
                    "margin-left: -2px;")
                .arg(m_wordColor));
        wordWithPunctLayout->addWidget(punctLabel);
      }

      wordWithPunctLayout->addStretch();

      wordLayout->addWidget(memberLabel);
      wordLayout->addLayout(wordWithPunctLayout);

    } else {
      // Для одиночных знаков препинания
      QVBoxLayout *punctLayout = new QVBoxLayout(container);
      punctLayout->setSpacing(0);
      punctLayout->setContentsMargins(2, 0, 2, 0);

      QLabel *spacerLabel = new QLabel("");
      spacerLabel->setFixedHeight(labelFm.height());

      QLabel *punctLabel = new QLabel(token);
      punctLabel->setAlignment(Qt::AlignCenter);
      punctLabel->setStyleSheet(
          QString("font-size: 11pt; color: %1; font-weight: normal;")
              .arg(m_wordColor));

      punctLayout->addWidget(spacerLabel);
      punctLayout->addWidget(punctLabel);
    }

    m_flowLayout->addWidget(container);
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

QString TextMarkupWidget::highlightedRole() const {
    return m_highlightedRole;
}


void TextMarkupWidget::updateHighlighting() {
    if (m_text.isEmpty()) {
        return;
    }


    // Проходим по всем виджетам внутри m_flowLayout
    for (int i = 0; i < m_flowLayout->count(); ++i) {
        QWidget *container = m_flowLayout->itemAt(i)->widget();
        if (!container) continue;

        // Ищем лейбл с ролью (он должен быть первым в layout контейнера)
        QVBoxLayout *vLayout = qobject_cast<QVBoxLayout*>(container->layout());
        if (!vLayout) continue;

        QLabel *memberLabel = qobject_cast<QLabel*>(vLayout->itemAt(0)->widget());
        if (!memberLabel) continue;

        // Ищем лейбл со словом (он должен быть в layout, который идет вторым)
        QHBoxLayout *hLayout = qobject_cast<QHBoxLayout*>(vLayout->itemAt(1)->layout());
        if (!hLayout) continue;

        QLabel *wordLabel = nullptr;
        for (int j = 0; j < hLayout->count(); ++j) {
            wordLabel = qobject_cast<QLabel*>(hLayout->itemAt(j)->widget());
            if (wordLabel) break; // Нашли первый QLabel (это слово)
        }

        if (!wordLabel) continue;

        // Проверяем, совпадает ли роль с той, которую нужно выделить
        if (memberLabel->text() == m_highlightedRole) {
            // Применяем выделение
            wordLabel->setStyleSheet(
                QString("font-size: 11pt; color: %1; font-weight: normal; "
                        "background-color: %2; padding: 1px; border-radius: 2px;")
                    .arg(m_wordColor)
                    .arg(m_wordBackgroundColor));
        } else {
            // Убираем выделение (делаем фон прозрачным)
            wordLabel->setStyleSheet(
                QString("font-size: 11pt; color: %1; font-weight: normal; "
                        "background-color: transparent;")
                    .arg(m_wordColor));
        }
    }
}
