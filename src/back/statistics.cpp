#include "statistics.hpp"
#include "bert_onnx_inference.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
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
 * категории встречаются по одному разу — нет «самого популярного»), возвращается
 * пара ("нет", 0).
 */
static std::pair<std::string, int>
pickTopWordOrNone(const std::unordered_map<std::string, int> &freq) {
  const std::pair<std::string, int> best = pickTopWord(freq);
  if (freq.empty() || best.second <= 1)
    return {"нет", 0};
  return best;
}

/**
 * @brief Добавляет строку в вектор, если её там ещё нет.
 * @param strings Вектор уникальных строк.
 * @param value Новое значение.
 */
static void pushUniqueIfMissing(std::vector<std::string> &strings,
                                const std::string &value) {
  if (std::find(strings.begin(), strings.end(), value) == strings.end()) {
    strings.push_back(value);
  }
}

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
  }
}

/**
 * @brief Увеличивает частотный счётчик слова для нужной категории.
 * @param categoryRu Название категории на русском.
 * @param word Текст сущности.
 * @param subjectFreq Словарь подлежащих.
 * @param predicateFreq Словарь сказуемых.
 * @param definitionFreq Словарь определений.
 * @param additionFreq Словарь дополнений.
 * @param adverbialFreq Словарь обстоятельств.
 */
static void
incrementTopCounter(const std::string &categoryRu, const std::string &word,
                    std::unordered_map<std::string, int> &subjectFreq,
                    std::unordered_map<std::string, int> &predicateFreq,
                    std::unordered_map<std::string, int> &definitionFreq,
                    std::unordered_map<std::string, int> &additionFreq,
                    std::unordered_map<std::string, int> &adverbialFreq) {
  if (categoryRu == "подлежащее") {
    subjectFreq[word] += 1;
  } else if (categoryRu == "сказуемое") {
    predicateFreq[word] += 1;
  } else if (categoryRu == "определение") {
    definitionFreq[word] += 1;
  } else if (categoryRu == "дополнение") {
    additionFreq[word] += 1;
  } else if (categoryRu == "обстоятельство") {
    adverbialFreq[word] += 1;
  }
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

  // По одному частотному словарю на категорию: текст фрагмента -> сколько раз
  // встретился.
  std::unordered_map<std::string, int> subjectFreq;
  std::unordered_map<std::string, int> predicateFreq;
  std::unordered_map<std::string, int> definitionFreq;
  std::unordered_map<std::string, int> additionFreq;
  std::unordered_map<std::string, int> adverbialFreq;

  for (const auto &sentenceResult : analysis_results) {
    stats.words_total += countWordsInText(sentenceResult.text);

    for (const auto &entity : sentenceResult.entities) {
      const std::string word = entity.text;
      const std::string categoryRu = entity.type_ru;
      if (word.empty() || categoryRu.empty()) {
        continue;
      }

      // Одна сущность = один член предложения в общей статистике.
      stats.members_total += 1;
      incrementCategoryTotal(stats, categoryRu);
      incrementTopCounter(categoryRu, word, subjectFreq, predicateFreq,
                          definitionFreq, additionFreq, adverbialFreq);
    }
  }

  // Для каждой категории — самый частый фрагмент; если все по разу (max==1) —
  // «нет».
  stats.top_subject = pickTopWordOrNone(subjectFreq);
  stats.top_predicate = pickTopWordOrNone(predicateFreq);
  stats.top_definition = pickTopWordOrNone(definitionFreq);
  stats.top_addition = pickTopWordOrNone(additionFreq);
  stats.top_adverbial = pickTopWordOrNone(adverbialFreq);

  return stats;
}

/**
 * @brief Строит агрегированный список элементов для страницы поиска.
 * @param analysis_results
 * @return Вектор SearchItem
 */
std::vector<SearchItem>
build_search_items(const std::vector<SentenceResult> &analysis_results) {
  // Ключ: (type_ru, текст фрагмента) — одна запись на уникальную пару по всему
  // тексту.
  std::map<std::pair<std::string, std::string>, SearchItem> itemsByKey;

  for (const SentenceResult &sentenceResult : analysis_results) {
    for (const Entity &entity : sentenceResult.entities) {
      const std::string word = entity.text;
      const std::string categoryRu = entity.type_ru;
      std::cout << entity.text << std::endl;
      if (word.empty() || categoryRu.empty()) {
        continue;
      }

      // operator[] создаёт SearchItem при первом появлении ключа.
      SearchItem &item = itemsByKey[{categoryRu, word}];
      item.text = word;
      item.type = categoryRu;
      item.amount += 1;
      // Разные предложения, где встретилась эта пара; без дублей одной строки.
      pushUniqueIfMissing(item.sentences, sentenceResult.text);
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
