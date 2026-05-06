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
 * - Фильтрацию по тексту поиска (по началу слова + точное совпадение)
 * - Фильтрацию по членам предложения
 * - Сортировку результатов с приоритетом точного совпадения
 *
 * Класс не зависит от Qt-виджетов и может быть протестирован изолированно.
 */
class SearchFilterModel {
public:
  /**
   * @brief Режимы сортировки результатов поиска.
   */
  enum SortMode {
    AlphabetAsc = 0,   /**< По алфавиту: от А к Я */
    AlphabetDesc = 1,  /**< По алфавиту: от Я к А */
    FrequencyDesc = 2, /**< По частоте: по убыванию */
    FrequencyAsc = 3   /**< По частоте: по возрастанию */
  };

  /**
   * @brief Стандартный набор всех поддерживаемых членов предложения.
   */
  static const QStringList kAllMembers;

  SearchFilterModel() = default;

  /**
   * @brief Установить исходный набор элементов для поиска.
   * @param items Вектор элементов, среди которых будет производиться поиск.
   */
  void setItems(const std::vector<SearchItem> &items);

  /**
   * @brief Применить фильтры и сортировку к данным.
   *
   * Выполняет:
   * 1. Фильтрацию по тексту (поиск по началу слова)
   * 2. Фильтрацию по членам предложения
   * 3. Сортировку с приоритетом точного совпадения
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
   *
   * Возвращает true в двух случаях:
   * - Текст элемента точно совпадает с запросом
   * - Текст элемента начинается с запроса
   *
   * @param item Проверяемый элемент.
   * @param needle Поисковый запрос (уже обрезанный от пробелов).
   * @return true, если элемент соответствует запросу.
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
   * При любом режиме сортировки точные совпадения всегда помещаются
   * в начало списка.
   *
   * @param results Вектор для сортировки (изменяется на месте).
   * @param sortMode Режим сортировки.
   * @param searchText Поисковый запрос для определения точных совпадений.
   */
  static void sortResults(std::vector<SearchItem> &results, SortMode sortMode,
                          const QString &searchText);

  /**
   * @brief Сравнить два элемента с приоритетом точного совпадения.
   *
   * Если один из элементов точно совпадает с запросом, а второй нет —
   * точное совпадение идёт первым. Если оба точные или оба нет —
   * применяется обычная сортировка.
   *
   * @param a Первый элемент для сравнения.
   * @param b Второй элемент для сравнения.
   * @param needle Поисковый запрос в нижнем регистре.
   * @param ascending true для сортировки А→Я, false для Я→А.
   * @return true, если a должен идти перед b.
   */
  static bool compareWithExactFirst(const SearchItem &a, const SearchItem &b,
                                    const QString &needle, bool ascending);

  /**
   * @brief Получить ключ для алфавитной сортировки элемента.
   * @param item Элемент для получения ключа.
   * @return Текст элемента в нижнем регистре.
   */
  static QString wordKey(const SearchItem &item);

  std::vector<SearchItem> allItems_;
};

#endif // SEARCHFILTERMODEL_HPP
