#ifndef SEARCHRESULTSLIST_HPP
#define SEARCHRESULTSLIST_HPP

#include <QFrame>
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
   * @brief Создать секцию с информацией о контекстах использования.
   * @param sentences Вектор пар (номер предложения, текст контекста).
   * @param parent Родительский виджет для создаваемых элементов.
   * @return Указатель на созданный виджет с контекстами.
   */
  QWidget *createSentencesSection(
      const std::vector<std::pair<int, std::string>> &sentences,
      QWidget *parent);

  /** @brief Контейнер для карточек внутри scroll area */
  QWidget *container_ = nullptr;

  /** @brief Layout для размещения карточек */
  QVBoxLayout *layout_ = nullptr;

  /** @brief Область прокрутки */
  QScrollArea *scrollArea_ = nullptr;
};

#endif // SEARCHRESULTSLIST_HPP
