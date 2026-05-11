#ifndef SEARCHPAGE_HPP
#define SEARCHPAGE_HPP

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <QWidget>
#include <vector>

#include "../back/statistics.hpp"
#include "SearchFilterCore.hpp"
#include "lib/SearchResultsList.hpp"

/**
 * @brief Страница поиска по результатам анализа.
 *
 * Предоставляет интерфейс для поиска и фильтрации результатов
 * синтаксического анализа текста. Включает:
 * - Поле ввода текста для поиска (живой поиск)
 * - Выпадающий список для фильтрации по членам предложения
 * - Выпадающий список для выбора режима сортировки
 * - Прокручиваемый список результатов в виде карточек
 * - Кнопку возврата на страницу результатов анализа
 *
 * Связь с другими компонентами:
 * - Получает данные от MainWindow через setSearchItems()
 * - Сигнализирует о необходимости возврата через backRequested()
 * - Использует SearchFilterModel для логики фильтрации
 * - Отображает результаты через SearchResultsList
 */
class SearchPage : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Конструктор страницы поиска.
   *
   * Создаёт и настраивает все UI-компоненты и связи сигналов-слотов.
   * Начальное состояние: показаны все члены предложения, сортировка А→Я.
   *
   * @param parent Родительский виджет (по умолчанию nullptr).
   */
  explicit SearchPage(QWidget *parent = nullptr);

  /**
   * @brief Установить данные для поиска.
   *
   * Заменяет текущий набор данных и немедленно применяет
   * активные фильтры для обновления отображения.
   *
   * @param items Вектор элементов для поиска (обычно из build_search_items()).
   */
  void setSearchItems(const std::vector<SearchItem> &items);

signals:
  /**
   * @brief Сигнал запроса на возврат к странице результатов.
   *
   * Испускается при нажатии кнопки "Назад".
   * MainWindow должен переключить стек на ResultPage.
   */
  void backRequested();

private slots:
  /**
   * @brief Обработчик изменения текста в поле поиска.
   *
   * Реализует "живой поиск" — фильтрация применяется при каждом
   * изменении текста без необходимости нажимать Enter.
   *
   * @param text Текущий текст из поля поиска.
   */
  void onSearchTextChanged(const QString &text);

  /**
   * @brief Обработчик изменения выбранного члена предложения.
   *
   * Обновляет список выбранных членов и переприменяет фильтры.
   */
  void onMemberFilterChanged();

  /**
   * @brief Обработчик изменения режима сортировки.
   *
   * @param index Новый индекс выбранного элемента в комбобоксе сортировки.
   */
  void onSortModeChanged(int index);

private:
  /**
   * @brief Создать и настроить все UI-компоненты.
   *
   * Выделено в отдельный метод для улучшения читаемости конструктора.
   * Создаёт виджеты и устанавливает layout'ы.
   */
  void setupUI();

  /**
   * @brief Создать кнопку "Назад" в верхней части страницы.
   * @return Указатель на созданную кнопку.
   */
  QPushButton *createBackButton();

  /**
   * @brief Создать поле ввода поискового запроса.
   * @return Указатель на созданное поле ввода.
   */
  QLineEdit *createSearchField();

  /**
   * @brief Создать выпадающий список фильтрации по членам предложения.
   * @return Указатель на созданный комбобокс.
   */
  QComboBox *createMemberFilterCombo();

  /**
   * @brief Создать выпадающий список выбора режима сортировки.
   * @return Указатель на созданный комбобокс.
   */
  QComboBox *createSortCombo();

  /**
   * @brief Создать строку с фильтрами (члены предложения + сортировка).
   * @return Указатель на виджет, содержащий фильтры в одной строке.
   */
  QWidget *createFilterRow();

  /**
   * @brief Создать центральную область с поиском, фильтрами и результатами.
   * @return Указатель на виджет центральной области.
   */
  QWidget *createContentArea();

  /**
   * @brief Установить все соединения сигналов и слотов.
   */
  void setupConnections();

  /**
   * @brief Применить текущие фильтры и обновить отображение.
   *
   * Основной метод обновления интерфейса. Собирает текущие значения
   * фильтров, передаёт их в SearchFilterModel и обновляет SearchResultsList.
   */
  void applyCurrentFilters();

  /**
   * @brief Поле ввода поискового запроса
   */
  QLineEdit *searchEdit_ = nullptr;

  /**
   * @brief Выпадающий список фильтрации по членам предложения
   */
  QComboBox *memberFilterCombo_ = nullptr;

  /**
   * @brief Выпадающий список выбора сортировки
   */
  QComboBox *sortCombo_ = nullptr;

  /**
   * @brief Кнопка возврата на страницу результатов
   */
  QPushButton *backButton_ = nullptr;

  /**
   * @brief Виджет прокручиваемого списка результатов
   */
  SearchResultsList *resultsList_ = nullptr;

  /**
   * @brief Модель фильтрации данных
   */
  SearchFilterCore filterModel_;

  /** @brief Текущий набор выбранных членов предложения */
  QStringList selectedMembers_;
};

#endif // SEARCHPAGE_HPP
