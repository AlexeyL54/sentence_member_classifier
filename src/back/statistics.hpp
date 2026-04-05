#ifndef STATISTICS_H
#define STATISTICS_H

#include "bert_onnx_inference.hpp"

#include <string>
#include <utility>
#include <vector>

/**
 * @brief Элемент агрегированного списка членов предложения для страницы поиска.
 */
struct SearchItem {
  std::string text; // член предложения
  std::string type; // вид члена предложения (подлежащее, сказуемое, ...)
  std::vector<std::string>
      sentences;  // предложения, в которых встречается этот член предложения
  int amount = 0; // количество появлений в тексте
};

/**
 * @brief Глобальная статистика по обработанному тексту (экран анализа).
 */
struct GlobalStats {
  int sentences_total = 0; // количество предложений в тексте
  int words_total =
      0; // количество слов в тексте (подсчёт по SentenceResult::text)
  int members_total = 0;     // количество членов предложения в тексте
  int subjects_total = 0;    // количество подлежащих в тексте
  int predicates_total = 0;  // количество сказуемых в тексте
  int definitions_total = 0; // количество определений в тексте
  int additions_total = 0;   // количество дополнений в тексте
  int adverbials_total = 0;  // количество обстоятельств в тексте
  std::pair<std::string, int> top_subject;    // самое популярное подлежащее
  std::pair<std::string, int> top_predicate;  // самое популярное сказуемое
  std::pair<std::string, int> top_definition; // самое популярное определение
  std::pair<std::string, int> top_addition;   // самое популярное дополнение
  std::pair<std::string, int> top_adverbial;  // самое популярное обстоятельство
};

/** Агрегаты по тексту из результата extract_sentence_parts. */
GlobalStats
build_global_stats(const std::vector<SentenceResult> &analysis_results);

/** Список SearchItem для SearchPage из того же результата анализа. */
std::vector<SearchItem>
build_search_items(const std::vector<SentenceResult> &analysis_results);

#endif // !STATISTICS_H
