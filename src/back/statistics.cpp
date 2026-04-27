#include "statistics.hpp"
#include "bert_onnx_inference.hpp"
#include "unistring.hpp"

#include <cctype>
#include <map>
#include <unordered_map>

/**
 * @brief Подсчитывает количество слов в строке предложения.
 * @param text Исходный текст предложения.
 * @return Количество непрерывных последовательностей непробельных символов.
 */
static int countWordsInText(const std::string &text) {
  int wordCount = 0;
  bool insideWord = false;
  for (unsigned char c : text) {
    if (std::isspace(c)) {
      insideWord = false;
    } else if (!insideWord) {
      // Начало нового «слова»: не пробел после пробела/начала строки.
      ++wordCount;
      insideWord = true;
    }
  }
  return wordCount;
}

/**
 * @brief Возвращает наиболее частое слово для категории.
 * @param freq Частотный словарь слово -> количество.
 * @return Пара (слово, количество); при равенстве частоты выбирается
 *         лексикографически меньшее слово.
 */
static std::pair<std::string, int>
pickTopWord(const std::unordered_map<std::string, int> &freq) {
  std::pair<std::string, int> best{"", 0};
  for (const auto &[word, count] : freq) {
    // Сначала по частоте; при равенстве — лексикографически меньшее слово.
    if (count > best.second || (count == best.second && word < best.first)) {
      best = {word, count};
    }
  }
  return best;
}

/**
 * @brief Самый частый фрагмент по категории или «нет», если выделить нельзя.
 *
 * Если категория пуста или максимальная частота равна 1 (все члены этой
 * категории встречаются по одному разу — нет «самого популярного»),
 * возвращается пара ("-", 0).
 */
static std::pair<std::string, int>
pickTopWordOrNone(const std::unordered_map<std::string, int> &freq) {
  const std::pair<std::string, int> best = pickTopWord(freq);
  if (freq.empty() || best.second <= 1)
    return {"-", 0};
  return best;
}

/**
 * @brief Добавляет контекст предложения, если для этого номера записи ещё нет.
 */
static void
pushSentenceContextIfMissing(std::vector<std::pair<int, std::string>> &contexts,
                             int sentence_number, const std::string &text) {
  for (const auto &c : contexts) {
    if (c.first == sentence_number)
      return;
  }
  contexts.push_back({sentence_number, text});
}

/**
 * @brief Частоты слов по категориям (для топов на экране статистики).
 */
struct CategoryWordFreqs {
  std::unordered_map<std::string, int> subject;
  std::unordered_map<std::string, int> predicate;
  std::unordered_map<std::string, int> definition;
  std::unordered_map<std::string, int> addition;
  std::unordered_map<std::string, int> adverbial;
  std::unordered_map<std::string, int> other;

  void increment(const std::string &categoryRu, const std::string &word) {
    if (categoryRu == "подлежащее") {
      subject[word] += 1;
    } else if (categoryRu == "сказуемое") {
      predicate[word] += 1;
    } else if (categoryRu == "определение") {
      definition[word] += 1;
    } else if (categoryRu == "дополнение") {
      addition[word] += 1;
    } else if (categoryRu == "обстоятельство") {
      adverbial[word] += 1;
    } else if (categoryRu == "другое") {
      other[word] += 1;
    }
  }
};

/**
 * @brief Увеличивает счётчик общей статистики для указанной категории.
 * @param stats Глобальная статистика.
 * @param categoryRu Название категории на русском.
 */
static void incrementCategoryTotal(GlobalStats &stats,
                                   const std::string &categoryRu) {
  if (categoryRu == "подлежащее") {
    stats.subjects_total += 1;
  } else if (categoryRu == "сказуемое") {
    stats.predicates_total += 1;
  } else if (categoryRu == "определение") {
    stats.definitions_total += 1;
  } else if (categoryRu == "дополнение") {
    stats.additions_total += 1;
  } else if (categoryRu == "обстоятельство") {
    stats.adverbials_total += 1;
  } else if (categoryRu == "другое") {
    stats.others_total += 1;
  }
}

/**
 * @brief Заполняет поля top_* из накопленных частот.
 */
static void fillTopStats(GlobalStats &stats, const CategoryWordFreqs &freqs) {
  stats.top_subject = pickTopWordOrNone(freqs.subject);
  stats.top_predicate = pickTopWordOrNone(freqs.predicate);
  stats.top_definition = pickTopWordOrNone(freqs.definition);
  stats.top_addition = pickTopWordOrNone(freqs.addition);
  stats.top_adverbial = pickTopWordOrNone(freqs.adverbial);
  stats.top_other = pickTopWordOrNone(freqs.other);
}

/**
 * @brief Учёт одной сущности в глобальной статистике и частотах по категориям.
 */
static void accumulateEntityForGlobalStats(GlobalStats &stats,
                                           CategoryWordFreqs &freqs,
                                           const Entity &entity) {
  std::string word = entity.text;
  const std::string categoryRu = entity.type_ru;
  if (word.empty() || categoryRu.empty()) {
    return;
  }

  // Приводим к нижнему регистру для группировки
  utf8::Unistring wordLower = utf8::Unistring(word).to_lower();
  const std::string wordKey = wordLower.to_string();

  // Одна сущность = один член предложения в общей статистике.
  stats.members_total += 1;
  incrementCategoryTotal(stats, categoryRu);
  freqs.increment(categoryRu, wordKey);
}

/**
 * @brief Строит глобальную статистику по результатам анализа предложений.
 * @param analysis_results Результат extract_sentence_parts().
 * @return Заполненная структура GlobalStats.
 */
GlobalStats
build_global_stats(const std::vector<SentenceResult> &analysis_results) {
  GlobalStats stats{};

  stats.sentences_total = static_cast<int>(analysis_results.size());

  CategoryWordFreqs freqs;

  for (const auto &sentenceResult : analysis_results) {
    stats.words_total += countWordsInText(sentenceResult.text);

    for (const auto &entity : sentenceResult.entities) {
      accumulateEntityForGlobalStats(stats, freqs, entity);
    }
  }

  // Для каждой категории — самый частый фрагмент; если все по разу (max==1) —
  // «нет».
  fillTopStats(stats, freqs);

  return stats;
}

/**
 * @brief Ключ слова в нижнем регистре для агрегации SearchItem.
 */
static std::string normalizedWordKey(const std::string &word) {
  utf8::Unistring wordLower = utf8::Unistring(word).to_lower();
  return wordLower.to_string();
}

/**
 * @brief Добавляет вхождение сущности в агрегат по ключу (категория, слово).
 */
static void mergeEntityIntoSearchItems(
    std::map<std::pair<std::string, std::string>, SearchItem> &itemsByKey,
    int sentence_number, const std::string &sentenceText,
    const Entity &entity) {
  std::string word = entity.text;
  const std::string categoryRu = entity.type_ru;
  if (word.empty() || categoryRu.empty()) {
    return;
  }

  const std::string wordKey = normalizedWordKey(word);

  // operator[] создаёт SearchItem при первом появлении ключа.
  SearchItem &item = itemsByKey[{categoryRu, wordKey}];

  // Сохраняем оригинальный текст при первом вхождении
  if (item.amount == 0) {
    item.text = word;
    item.type = categoryRu;
  }

  item.amount += 1;
  // Одно предложение на номер: без дубликатов при нескольких сущностях
  // в одном предложении.
  pushSentenceContextIfMissing(item.sentences, sentence_number, sentenceText);
}

/**
 * @brief Строит агрегированный список элементов для страницы поиска.
 * @param analysis_results
 * @return Вектор SearchItem
 */
std::vector<SearchItem>
build_search_items(const std::vector<SentenceResult> &analysis_results) {
  // Ключ: (type_ru, текст фрагмента в нижнем регистре) — одна запись на
  // уникальную пару по всему тексту.
  std::map<std::pair<std::string, std::string>, SearchItem> itemsByKey;

  for (size_t si = 0; si < analysis_results.size(); ++si) {
    const auto &sentenceResult = analysis_results[si];
    const int sentence_number = static_cast<int>(si) + 1;

    for (const auto &entity : sentenceResult.entities) {
      mergeEntityIntoSearchItems(itemsByKey, sentence_number,
                                 sentenceResult.text, entity);
    }
  }

  // map -> vector: порядок следует сравнению ключей (type, затем text).
  std::vector<SearchItem> items;
  items.reserve(itemsByKey.size());
  for (auto &[key, item] : itemsByKey) {
    (void)key; // ключ уже отражён в полях item; нужен только для обхода map
    items.push_back(std::move(item));
  }
  return items;
}
