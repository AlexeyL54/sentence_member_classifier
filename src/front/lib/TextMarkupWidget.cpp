#include "TextMarkupWidget.hpp"
#include "FlowLayout.hpp"
// Убедитесь, что этот заголовок подключен, так как мы используем Unistring
#include "../../back/unistring.hpp"
#include <QFontMetrics>
#include <QRegularExpression>
#include <QVBoxLayout>

/**
 * @brief Конструктор класса TextMarkupWidget.
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
 * @brief Устанавливает вектор с данными для отображения.
 * @param results Вектор с данными (текст предложения, слова и роли).
 */
void TextMarkupWidget::setMarkupText(std::vector<SentenceResult> &results) {
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
 * @brief Возвращает текущий цвет подсветки.
 * @return Цвет подсветки в формате CSS.
 */
QString TextMarkupWidget::wordBackgroundColor() const {
  return m_wordBackgroundColor;
}

/**
 * @brief Устанавливает член предложения для подсветки.
 * @param role - название члена предложения.
 */
void TextMarkupWidget::setHighlightedRole(const QString &role) {
  if (m_highlightedRole != role) {
    m_highlightedRole = role;
    updateHighlighting();
  }
}

/**
 * @brief Возвращает текущий член предложения для подсветки.
 * @return Название члена предложения.
 */
QString TextMarkupWidget::highlightedRole() const { return m_highlightedRole; }

/**
 * @brief Подсвечивает выбранные члены предложения.
 */
void TextMarkupWidget::updateHighlighting() {
  /*if (m_text.isEmpty()) {
    return;
  }*/
  for (int i = 0; i < m_flowLayout->count(); ++i) {
    QWidget *container = m_flowLayout->itemAt(i)->widget();
    if (!container)
      continue;
    QVBoxLayout *vLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!vLayout || vLayout->count() < 2)
      continue;
    QLabel *memberLabel = qobject_cast<QLabel *>(vLayout->itemAt(0)->widget());
    if (!memberLabel)
      continue;
    QLabel *wordLabel = qobject_cast<QLabel *>(vLayout->itemAt(1)->widget());
    if (!wordLabel)
      continue;
    if (memberLabel->text().isEmpty()) {
      continue;
    }
    if (memberLabel->text() == m_highlightedRole) {
      wordLabel->setStyleSheet(
          QString("font-size: 11pt; color: %1; font-weight: normal; "
                  "background-color: %2; padding: 1px; border-radius: 2px;")
              .arg(m_wordColor)
              .arg(m_wordBackgroundColor));
    } else {
      wordLabel->setStyleSheet(
          QString("font-size: 11pt; color: %1; font-weight: normal; "
                  "background-color: transparent;")
              .arg(m_wordColor));
    }
  }
}

/**
 * @brief Создает виджет для знака препинания или другого одиночного символа.
 * @param charStr Строка UTF-8 с символом.
 * @param containerHeight Высота контейнера.
 * @return Указатель на созданный виджет.
 */
QWidget *TextMarkupWidget::createPunctuationWidget(const std::string &charStr,
                                                   int containerHeight) {
  QWidget *punctContainer = new QWidget();
  punctContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
  punctContainer->setFixedHeight(containerHeight);

  QVBoxLayout *punctLayout = new QVBoxLayout(punctContainer);
  punctLayout->setSpacing(0);
  punctLayout->setContentsMargins(0, 0, 2, 0);

  QLabel *emptyLabel = new QLabel("");
  QLabel *charLabel = new QLabel(QString::fromUtf8(charStr.c_str()));
  charLabel->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
  charLabel->setStyleSheet(
      QString("font-size: 11pt; color: %1;").arg(m_wordColor));

  punctLayout->addWidget(emptyLabel);
  punctLayout->addWidget(charLabel);
  return punctContainer;
}

/**
 * @brief Создает виджет для слова с подписью члена предложения.
 * @param entityText Текст слова.
 * @param shortRole Краткая роль члена предложения.
 * @param fullRole Полная роль члена предложения (для tooltip).
 * @param containerHeight Высота контейнера.
 * @return Указатель на созданный виджет.
 */
QWidget *TextMarkupWidget::createWordWidget(const QString &entityText,
                                            const QString &shortRole,
                                            const QString &fullRole,
                                            int containerHeight) {
  QWidget *wordContainer = new QWidget();
  wordContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  wordContainer->setFixedHeight(containerHeight);

  QVBoxLayout *blockLayout = new QVBoxLayout(wordContainer);
  blockLayout->setSpacing(0);
  blockLayout->setContentsMargins(2, 0, 2, 0);

  QLabel *memberLabel = new QLabel(shortRole);
  memberLabel->setAlignment(Qt::AlignCenter);
  memberLabel->setStyleSheet(
      QString("font-size: 7pt; color: %1;").arg(m_labelColor));

  QLabel *wordLabel = new QLabel(entityText);
  wordLabel->setAlignment(Qt::AlignCenter | Qt::AlignLeft);
  wordLabel->setStyleSheet(
      QString("font-size: 11pt; color: %1;").arg(m_wordColor));
  wordLabel->setToolTip(fullRole);

  blockLayout->addWidget(memberLabel);
  blockLayout->addWidget(wordLabel);
  return wordContainer;
}

/**
 * @brief Обрабатывает промежуток текста между сущностями.
 * @param uniGap Подстрока Unistring с текстом промежутка.
 * @param containerHeight Высота контейнера.
 */
void TextMarkupWidget::processGap(const utf8::Unistring &uniGap,
                                  int containerHeight) {
  for (size_t k = 0; k < uniGap.length(); ++k) {
    std::string charStr = uniGap[k].to_string();
    QChar qch = QString::fromUtf8(charStr.c_str()).at(0);

    if (qch.isSpace()) {
      continue;
    }

    QWidget *punctContainer = createPunctuationWidget(charStr, containerHeight);
    m_flowLayout->addWidget(punctContainer);
  }
}

/**
 * @brief Обрабатывает хвост строки после последней сущности.
 * @param uniTail Подстрока Unistring с текстом хвоста.
 * @param containerHeight Высота контейнера.
 */
void TextMarkupWidget::processTail(const utf8::Unistring &uniTail,
                                   int containerHeight) {
  for (size_t k = 0; k < uniTail.length(); ++k) {
    std::string charStr = uniTail[k].to_string();
    QChar qch = QString::fromUtf8(charStr.c_str()).at(0);

    // Пропускаем пробелы
    if (qch.isSpace())
      continue;

    // Обрабатываем все непустые символы
    QWidget *punctContainer = createPunctuationWidget(charStr, containerHeight);
    m_flowLayout->addWidget(punctContainer);
  }
}

/**
 * @brief Перестраивает отображение текста, корректно обрабатывая UTF-8.
 */
void TextMarkupWidget::rebuild() {
  clearLayout();
  if (m_results.empty()) {
    return;
  }

  QMap<QString, QString> roleMap = createRoleMap();
  int containerHeight = calculateContainerHeight();

  for (const auto &sentence : m_results) {
    processSentence(sentence, containerHeight, roleMap);
  }

  update();
  m_scrollArea->widget()->update();
}

/**
 * @brief Очищает текущий layout виджета.
 */
void TextMarkupWidget::clearLayout() {
  QLayoutItem *child;
  while ((child = m_flowLayout->takeAt(0)) != nullptr) {
    delete child->widget();
    delete child;
  }
}

/**
 * @brief Вычисляет высоту контейнера для слов и подписей.
 * @return Высота контейнера в пикселях.
 */
int TextMarkupWidget::calculateContainerHeight() {
  QFont wordFont("", 11);
  QFontMetrics wordFm(wordFont);
  QFont labelFont("", 7);
  QFontMetrics labelFm(labelFont);
  return wordFm.height() + labelFm.height() + 6;
}

/**
 * @brief Создает мапу соответствия полных и кратких ролей.
 * @return Мапа ролей.
 */
QMap<QString, QString> TextMarkupWidget::createRoleMap() {
  static const QMap<QString, QString> roleMap = {
      {"подлежащее", "подл."},     {"сказуемое", "сказ."},
      {"обстоятельство", "обст."}, {"определение", "опр."},
      {"дополнение", "доп."},      {"другое", "др."}};
  return roleMap;
}

/**
 * @brief Преобразует байтовое смещение в символьный индекс.
 * @param bytePos Байтовое смещение.
 * @param byteOffsets Вектор смещений байтов.
 * @return Символьный индекс.
 */
size_t
TextMarkupWidget::byteToCharIndex(size_t bytePos,
                                  const std::vector<size_t> &byteOffsets) {
  if (bytePos == 0)
    return 0;
  for (size_t i = 0; i < byteOffsets.size(); ++i) {
    if (byteOffsets[i] >= bytePos) {
      return i;
    }
  }
  return byteOffsets.size();
}

/**
 * @brief Обрабатывает одно предложение с сущностями.
 * @param sentence Предложение с сущностями.
 * @param containerHeight Высота контейнера.
 * @param roleMap Мапа ролей.
 */
void TextMarkupWidget::processSentence(const SentenceResult &sentence,
                                       int containerHeight,
                                       const QMap<QString, QString> &roleMap) {
  const std::string &stdText = sentence.text;
  utf8::Unistring uniText(stdText);
  size_t textLenChars = uniText.length();
  std::vector<size_t> byteOffsets = uniText.get_char_offsets();
  size_t currentCharPos = 0;

  for (const auto &entity : sentence.entities) {
    size_t entityStartChar = byteToCharIndex(entity.start, byteOffsets);
    size_t entityEndChar = byteToCharIndex(entity.end, byteOffsets);

    if (entityStartChar > currentCharPos) {
      utf8::Unistring uniGap =
          uniText.substr(currentCharPos, entityStartChar - 1);
      processGap(uniGap, containerHeight);
    }

    if (entityEndChar <= currentCharPos || entityEndChar > textLenChars) {
      continue;
    }

    utf8::Unistring uniEntity =
        uniText.substr(entityStartChar, entityEndChar - 1);
    QString entityText = QString::fromUtf8(uniEntity.to_string().c_str());
    QString fullRole = QString::fromStdString(entity.type_ru);
    QString shortRole = roleMap.value(fullRole, fullRole);

    QWidget *wordContainer =
        createWordWidget(entityText, shortRole, fullRole, containerHeight);
    m_flowLayout->addWidget(wordContainer);
    currentCharPos = entityEndChar;
  }

  if (currentCharPos < textLenChars) {
    utf8::Unistring uniTail = uniText.substr(currentCharPos, textLenChars - 1);
    processTail(uniTail, containerHeight);
  }
}
