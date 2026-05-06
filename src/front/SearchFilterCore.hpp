#ifndef SEARCHFILTERMODEL_HPP
#define SEARCHFILTERMODEL_HPP

#include <QString>
#include <QStringList>
#include <vector>

#include "../back/statistics.hpp"

/**
 * @brief Модель данных и логики фильтрации для поиска.
 *
 * Инкапсулирует всю бизнес-логику фильтрации результатов анализа:
 * - Фильтрацию по тексту поиска
 * - Фильтрацию по членам предложения
 * - Сортировку результатов по различным критериям
 *
 * Класс не зависит от Qt-виджетов и может быть протестирован изолированно.
 */
class SearchFilterModel {
public:
  /**
   * @brief Режимы сортировки результатов поиска.
   */
  enum SortMode {
    AlphabetAsc = 0,   /** По алфавиту: от А к Я */
    AlphabetDesc = 1,  /** По алфавиту: от Я к А */
    FrequencyDesc = 2, /** По частоте: по убыванию */
    FrequencyAsc = 3   /** По частоте: по возрастанию */
  };

  /**
   * @brief Стандартный набор всех поддерживаемых членов предложения.
   *
   * Используется для инициализации UI и как значение по умолчанию
   * при выборе опции "Все члены предложения".
   */
  static const QStringList kAllMembers;

  /**
   * @brief Конструктор по умолчанию.
   */
  SearchFilterModel() = default;

  /**
   * @brief Установить исходный набор элементов для поиска.
   * @param items Вектор элементов, среди которых будет производиться поиск.
   */
  void setItems(const std::vector<SearchItem> &items);

  /**
   * @brief Применить фильтры и сортировку к данным.
   *
   * Выполняет последовательно:
   * 1. Фильтрацию по тексту (если текст не пустой)
   * 2. Фильтрацию по членам предложения
   * 3. Сортировку согласно выбранному режиму
   *
   * @param searchText Текст для поиска (может быть пустым).
   * @param selectedMembers Список выбранных членов предложения.
   * @param sortMode Режим сортировки результатов.
   * @return Отфильтрованный и отсортированный вектор элементов.
   */
  std::vector<SearchItem> applyFilters(const QString &searchText,
                                       const QStringList &selectedMembers,
                                       SortMode sortMode) const;

private:
  /**
   * @brief Проверить, соответствует ли элемент поисковому запросу.
   * @param item Проверяемый элемент.
   * @param needle Поисковый запрос (уже обрезанный от пробелов).
   * @return true, если текст элемента содержит needle (без учёта регистра).
   */
  static bool matchesText(const SearchItem &item, const QString &needle);

  /**
   * @brief Проверить, входит ли член предложения элемента в выбранные.
   * @param item Проверяемый элемент.
   * @param selectedMembers Список выбранных членов предложения.
   * @return true, если член предложения элемента есть в списке.
   */
  static bool matchesMember(const SearchItem &item,
                            const QStringList &selectedMembers);

  /**
   * @brief Отсортировать результаты согласно выбранному режиму.
   *
   * При равных значениях частоты используется алфавитная сортировка
   * как дополнительный критерий для стабильности порядка.
   *
   * @param results Вектор для сортировки (изменяется на месте).
   * @param sortMode Режим сортировки.
   */
  static void sortResults(std::vector<SearchItem> &results, SortMode sortMode);

  /**
   * @brief Получить ключ для алфавитной сортировки элемента.
   * @param item Элемент для получения ключа.
   * @return Текст элемента в нижнем регистре.
   */
  static QString wordKey(const SearchItem &item);

  /** @brief Исходный набор всех элементов для поиска. */
  std::vector<SearchItem> allItems_;
};

#endif // SEARCHFILTERMODEL_HPP
