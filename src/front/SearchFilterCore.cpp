#include "SearchFilterCore.hpp"

const QStringList SearchFilterModel::kAllMembers = {
    QStringLiteral("подлежащее"),     QStringLiteral("сказуемое"),
    QStringLiteral("дополнение"),     QStringLiteral("определение"),
    QStringLiteral("обстоятельство"), QStringLiteral("другое")};

void SearchFilterModel::setItems(const std::vector<SearchItem> &items) {
  allItems_ = items;
}

std::vector<SearchItem>
SearchFilterModel::applyFilters(const QString &searchText,
                                const QStringList &selectedMembers,
                                SortMode sortMode) const {
  std::vector<SearchItem> filtered;
  filtered.reserve(allItems_.size());

  const QString needle = searchText.trimmed();

  // Двухэтапная фильтрация: текст + член предложения
  for (const auto &item : allItems_) {
    if (!matchesText(item, needle))
      continue;
    if (!matchesMember(item, selectedMembers))
      continue;
    filtered.push_back(item);
  }

  sortResults(filtered, sortMode);
  return filtered;
}

bool SearchFilterModel::matchesText(const SearchItem &item,
                                    const QString &needle) {
  // Пустой запрос пропускает все элементы
  if (needle.isEmpty())
    return true;

  return QString::fromStdString(item.text).contains(needle,
                                                    Qt::CaseInsensitive);
}

bool SearchFilterModel::matchesMember(const SearchItem &item,
                                      const QStringList &selectedMembers) {
  // Пустой список — ничего не показываем (пользователь снял все галки)
  if (selectedMembers.isEmpty())
    return false;

  return selectedMembers.contains(QString::fromStdString(item.type));
}

void SearchFilterModel::sortResults(std::vector<SearchItem> &results,
                                    SortMode sortMode) {
  switch (sortMode) {
  case AlphabetAsc:
    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) {
      return wordKey(a) < wordKey(b);
    });
    break;

  case AlphabetDesc:
    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) {
      return wordKey(a) > wordKey(b);
    });
    break;

  case FrequencyDesc:
    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) {
      if (a.amount != b.amount)
        return a.amount > b.amount;
      // При равной частоте — алфавитный порядок
      return wordKey(a) < wordKey(b);
    });
    break;

  case FrequencyAsc:
    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) {
      if (a.amount != b.amount)
        return a.amount < b.amount;
      // При равной частоте — алфавитный порядок
      return wordKey(a) < wordKey(b);
    });
    break;
  }
}

QString SearchFilterModel::wordKey(const SearchItem &item) {
  return QString::fromStdString(item.text).toLower();
}
