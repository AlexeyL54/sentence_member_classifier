#include "SearchFilterCore.hpp"

#include <algorithm>

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

  for (const auto &item : allItems_) {
    if (!matchesText(item, needle))
      continue;
    if (!matchesMember(item, selectedMembers))
      continue;
    filtered.push_back(item);
  }

  sortResults(filtered, sortMode, needle);
  return filtered;
}

bool SearchFilterModel::matchesText(const SearchItem &item,
                                    const QString &needle) {
  if (needle.isEmpty())
    return true;

  const QString wordText = QString::fromStdString(item.text);

  if (wordText.compare(needle, Qt::CaseInsensitive) == 0)
    return true;

  return wordText.startsWith(needle, Qt::CaseInsensitive);
}

bool SearchFilterModel::matchesMember(const SearchItem &item,
                                      const QStringList &selectedMembers) {
  if (selectedMembers.isEmpty())
    return false;

  return selectedMembers.contains(QString::fromStdString(item.type));
}

void SearchFilterModel::sortResults(std::vector<SearchItem> &results,
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

bool SearchFilterModel::compareWithExactFirst(const SearchItem &a,
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

QString SearchFilterModel::wordKey(const SearchItem &item) {
  return QString::fromStdString(item.text).toLower();
}
