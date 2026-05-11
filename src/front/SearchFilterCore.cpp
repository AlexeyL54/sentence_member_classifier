#include "SearchFilterCore.hpp"

#include <algorithm>

const QStringList SearchFilterCore::kAllMembers = {
    QStringLiteral("подлежащее"),     QStringLiteral("сказуемое"),
    QStringLiteral("дополнение"),     QStringLiteral("определение"),
    QStringLiteral("обстоятельство"), QStringLiteral("другое")};

/**
 * @brief Установить исходный набор элементов для поиска.
 * @param items Вектор элементов, среди которых будет производиться поиск.
 */
void SearchFilterCore::setItems(const std::vector<SearchItem> &items) {
  allItems_ = items;
}

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
std::vector<SearchItem>
SearchFilterCore::applyFilters(const QString &searchText,
                               const QStringList &selectedMembers,
                               SortMode sortMode) const {
  std::vector<SearchItem> filtered;
  filtered.reserve(allItems_.size());

  const QString needle = searchText.trimmed();

  for (const SearchItem &item : allItems_) {
    if (!matchesText(item, needle))
      continue;
    if (!matchesMember(item, selectedMembers))
      continue;
    filtered.push_back(item);
  }

  sortResults(filtered, sortMode, needle);
  return filtered;
}

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
bool SearchFilterCore::matchesText(const SearchItem &item,
                                   const QString &needle) {
  if (needle.isEmpty())
    return true;

  const QString wordText = item.text;

  if (wordText.compare(needle, Qt::CaseInsensitive) == 0)
    return true;

  return wordText.startsWith(needle, Qt::CaseInsensitive);
}

/**
 * @brief Проверить, входит ли член предложения элемента в выбранные.
 * @param item Проверяемый элемент.
 * @param selectedMembers Список выбранных членов предложения.
 * @return true, если член предложения элемента есть в списке.
 */
bool SearchFilterCore::matchesMember(const SearchItem &item,
                                     const QStringList &selectedMembers) {
  if (selectedMembers.isEmpty())
    return false;

  return selectedMembers.contains(item.type);
}

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
void SearchFilterCore::sortResults(std::vector<SearchItem> &results,
                                   SortMode sortMode,
                                   const QString &searchText) {
  const QString needle = searchText.toLower();

  switch (sortMode) {
  case AlphabetAsc:
    std::sort(results.begin(), results.end(),
              [&needle](const auto &a, const auto &b) {
                return compareWithExactFirst(a, b, needle, true);
              });
    break;

  case AlphabetDesc:
    std::sort(results.begin(), results.end(),
              [&needle](const auto &a, const auto &b) {
                return compareWithExactFirst(a, b, needle, false);
              });
    break;

  case FrequencyDesc:
    std::sort(results.begin(), results.end(),
              [&needle](const auto &a, const auto &b) {
                if (a.amount != b.amount)
                  return a.amount > b.amount;
                return compareWithExactFirst(a, b, needle, true);
              });
    break;

  case FrequencyAsc:
    std::sort(results.begin(), results.end(),
              [&needle](const auto &a, const auto &b) {
                if (a.amount != b.amount)
                  return a.amount < b.amount;
                return compareWithExactFirst(a, b, needle, true);
              });
    break;
  }
}

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
bool SearchFilterCore::compareWithExactFirst(const SearchItem &a,
                                             const SearchItem &b,
                                             const QString &needle,
                                             bool ascending) {
  const QString keyA = wordKey(a);
  const QString keyB = wordKey(b);

  const bool aExact = (keyA == needle);
  const bool bExact = (keyB == needle);

  if (aExact && !bExact)
    return true;
  if (!aExact && bExact)
    return false;

  if (ascending)
    return keyA < keyB;
  else
    return keyA > keyB;
}

/**
 * @brief Получить ключ для алфавитной сортировки элемента.
 * @param item Элемент для получения ключа.
 * @return Текст элемента в нижнем регистре.
 */
QString SearchFilterCore::wordKey(const SearchItem &item) {
  return item.text.toLower();
}
