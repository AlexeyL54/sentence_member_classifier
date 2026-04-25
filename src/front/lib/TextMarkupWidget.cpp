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

void TextMarkupWidget::setMarkupText(std::vector<SentenceResult> &results) {
  m_results = results;
  rebuild();
}

void TextMarkupWidget::setWordColor(const QString &color) {
  if (m_wordColor != color) {
    m_wordColor = color;
    rebuild();
  }
}

void TextMarkupWidget::setLabelColor(const QString &color) {
  if (m_labelColor != color) {
    m_labelColor = color;
    rebuild();
  }
}

QString TextMarkupWidget::wordColor() const { return m_wordColor; }
QString TextMarkupWidget::labelColor() const { return m_labelColor; }
QString TextMarkupWidget::wordBackgroundColor() const {
  return m_wordBackgroundColor;
}

void TextMarkupWidget::setHighlightedRole(const QString &role) {
  if (m_highlightedRole != role) {
    m_highlightedRole = role;
    updateHighlighting();
  }
}

QString TextMarkupWidget::highlightedRole() const { return m_highlightedRole; }

void TextMarkupWidget::updateHighlighting() {
  if (m_text.isEmpty()) {
    return;
  }
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
 * @brief Перестраивает отображение текста, корректно обрабатывая UTF-8.
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

  // Высота блока: высота слова + высота лейбла + небольшие отступы
  int containerHeight = wordFm.height() + labelFm.height() + 6;

  for (const auto &sentence : m_results) {
    // Исходный текст предложения в виде std::string (UTF-8)
    const std::string &stdText = sentence.text;

    // Обертка Unistring для корректной работы с многобайтовыми символами
    utf8::Unistring uniText(stdText);
    size_t textLenChars = uniText.length();

    // Получаем смещения байтов для каждого символа
    std::vector<size_t> byteOffsets = uniText.get_char_offsets();

    // Функция для перевода байтового смещения в символьный индекс
    auto byteToCharIndex = [&](size_t bytePos) -> size_t {
      if (bytePos == 0)
        return 0;
      for (size_t i = 0; i < byteOffsets.size(); ++i) {
        if (byteOffsets[i] >= bytePos) {
          return i;
        }
      }
      return byteOffsets.size();
    };

    size_t currentCharPos = 0; // Текущая позиция в символах

    for (const auto &entity : sentence.entities) {
      // Переводим байтовые смещения в символьные индексы
      size_t entityStartChar = byteToCharIndex(entity.start);
      size_t entityEndChar = byteToCharIndex(entity.end);

      // 1. Обработка текста МЕЖДУ предыдущей сущностью и текущей
      if (entityStartChar > currentCharPos) {
        // Извлекаем промежуток как подстроку Unistring
        utf8::Unistring uniGap =
            uniText.substr(currentCharPos, entityStartChar - 1);

        // Выводим каждый символ отдельно
        for (size_t k = 0; k < uniGap.length(); ++k) {
          std::string charStr = uniGap[k].to_string();
          QChar qch = QString::fromUtf8(charStr.c_str()).at(0);

          if (qch.isSpace()) {
            continue;
          }

          QWidget *punctContainer = new QWidget();
          punctContainer->setSizePolicy(QSizePolicy::Fixed,
                                        QSizePolicy::Minimum);
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
          m_flowLayout->addWidget(punctContainer);
        }
      }

      // 2. Обработка самой СУЩНОСТИ
      if (entityEndChar <= currentCharPos || entityEndChar > textLenChars) {
        // Некорректная сущность, пропускаем
        continue;
      }

      // Извлекаем текст сущности через Unistring::substr (символьные индексы!)
      utf8::Unistring uniEntity =
          uniText.substr(entityStartChar, entityEndChar - 1);
      QString entityText = QString::fromUtf8(uniEntity.to_string().c_str());

      QString fullRole = QString::fromStdString(entity.type_ru);
      QString shortRole = roleMap.value(fullRole, fullRole);

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
      m_flowLayout->addWidget(wordContainer);

      // Обновляем текущую позицию на конец этой сущности
      currentCharPos = entityEndChar;
    }

    // 3. Обработка хвоста строки после последней сущности
    if (currentCharPos < textLenChars) {
      utf8::Unistring uniTail =
          uniText.substr(currentCharPos, textLenChars - 1);

      for (size_t k = 0; k < uniTail.length(); ++k) {
        std::string charStr = uniTail[k].to_string();
        QChar qch = QString::fromUtf8(charStr.c_str()).at(0);

        // Пропускаем пробелы
        if (qch.isSpace())
          continue;

        // ВАЖНО: Не пропускаем буквы и цифры!
        // Они могут остаться, если текст заканчивается без знака препинания
        // или если сущность не покрыла весь текст

        // Обрабатываем все непустые символы
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
        m_flowLayout->addWidget(punctContainer);
      }
    }
  }

  update();
  m_scrollArea->widget()->update();
}
