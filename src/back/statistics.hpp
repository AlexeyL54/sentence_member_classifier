#ifndef STATISTICS_H
#define STATISTICS_H

#include "bert_onnx_inference.hpp"
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Элемент агрегированного списка членов предложения для страницы поиска.
 * @param sentences пары (номер предложения в тексте, 1-based; полный текст).
 */
struct SearchItem {
  std::string text;
  std::string type;
  int amount = 0;
  std::vector<std::pair<int, std::string>> sentences;
};

/**
 * @brief Глобальная статистика по обработанному тексту (экран анализа).
 */
struct GlobalStats {
  int sentences_total = 0;
  int words_total = 0;
  int members_total = 0;
  int subjects_total = 0;
  int predicates_total = 0;
  int definitions_total = 0;
  int additions_total = 0;
  int adverbials_total = 0;
  int others_total = 0;
  std::pair<std::string, int> top_subject;
  std::pair<std::string, int> top_predicate;
  std::pair<std::string, int> top_definition;
  std::pair<std::string, int> top_addition;
  std::pair<std::string, int> top_adverbial;
  std::pair<std::string, int> top_other;
};

/** Агрегаты по тексту из результата extract_sentence_parts. */
GlobalStats
build_global_stats(const std::vector<SentenceResult> &analysis_results);

/** Список SearchItem для SearchPage из того же результата анализа. */
std::vector<SearchItem>
build_search_items(const std::vector<SentenceResult> &analysis_results);

#endif // !STATISTICS_H
