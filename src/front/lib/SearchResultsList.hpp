#ifndef SEARCHRESULTSLIST_HPP
#define SEARCHRESULTSLIST_HPP

#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>
#include <vector>

#include "../../back/statistics.hpp"

/**
 * @brief Виджет для отображения прокручиваемого списка результатов поиска.
 *
 * Создаёт карточки с информацией о каждом найденном элементе:
 * - Само слово (член предложения)
 * - Тип члена предложения
 * - Количество вхождений
 * - Контексты использования (номера предложений с preview текста)
 *
 * Поддерживает динамическое обновление содержимого через setItems().
 */
class SearchResultsList : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Конструктор виджета списка результатов.
   * @param parent Родительский виджет (по умолчанию nullptr).
   */
  explicit SearchResultsList(QWidget *parent = nullptr);

  /**
   * @brief Обновить содержимое списка новым набором элементов.
   *
   * Полностью заменяет текущее содержимое. Старые карточки уничтожаются
   * через deleteLater() для безопасной работы с событиями Qt.
   *
   * @param items Новый набор элементов для отображения.
   */
  void setItems(const std::vector<SearchItem> &items);

private:
  /**
   * @brief Удалить все карточки из layout'а.
   *
   * Безопасно очищает layout, планируя удаление виджетов
   * через очередь событий Qt.
   */
  void clearLayout();

  /**
   * @brief Создать одну карточку для элемента поиска.
   * @param item Элемент для отображения.
   * @return Указатель на созданный фрейм с карточкой.
   */
  QFrame *createCard(const SearchItem &item);

  /**
   * @brief Настроить основную компоновку виджета.
   */
  void setupUi();

  /**
   * @brief Настроить внешний вид карточки.
   * @param card Карточка для настройки.
   */
  void setupCardStyle(QFrame *card);

  /**
   * @brief Создать виджет с основным словом.
   * @param text Текст слова.
   * @param parent Родительский виджет.
   * @return Указатель на созданную метку.
   */
  QLabel *createWordLabel(const QString &text, QWidget *parent);

  /**
   * @brief Создать метку с типом члена предложения.
   * @param type Тип члена предложения.
   * @param parent Родительский виджет.
   * @return Указатель на созданную метку.
   */
  QLabel *createMemberTypeLabel(const QString &type, QWidget *parent);

  /**
   * @brief Создать метку с количеством вхождений.
   * @param amount Количество вхождений.
   * @param parent Родительский виджет.
   * @return Указатель на созданную метку.
   */
  QLabel *createCountLabel(int amount, QWidget *parent);

  /**
   * @brief Создать секцию с информацией о контекстах использования.
   * @param sentences Вектор пар (номер предложения, текст контекста).
   * @param parent Родительский виджет для создаваемых элементов.
   * @return Указатель на созданный виджет с контекстами.
   */
  QWidget *
  createSentencesSection(const std::vector<std::pair<int, QString>> &sentences,
                         QWidget *parent);

  /**
   * @brief Создать виджет для одного контекста.
   * @param sentencePair Пара (номер предложения, текст).
   * @param parent Родительский виджет.
   * @return Указатель на созданную метку или nullptr.
   */
  QLabel *createSentenceWidget(const std::pair<int, QString> &sentencePair,
                               QWidget *parent);

  /**
   * @brief Обрезать текст до максимальной длины.
   * @param text Исходный текст.
   * @return Обрезанный текст с многоточием при необходимости.
   */
  QString truncateSnippet(const QString &text) const;

  QWidget *container_ = nullptr;  // Контейнер для карточек внутри scroll area
  QVBoxLayout *layout_ = nullptr; // Layout для размещения карточек
  QScrollArea *scrollArea_ = nullptr; // Область прокрутки
};

#endif // SEARCHRESULTSLIST_HPP
