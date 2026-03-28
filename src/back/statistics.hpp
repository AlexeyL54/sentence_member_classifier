#ifndef STATISTICS_H
#define STATISTICS_H

#include <string>
#include <vector>

/**
 * @brief Элемент упорядоченного списка всех членов предложения, найденных в
 * тексте
 */
struct SearchItem {
  std::string text; // член предложения
  std::string type; // вид члена предлежония (подлежащее, сказуемое, ...)
  std::vector<std::string>
      sentences; // предложения, в которых встречается этот член предложения
  int amount;    // количество появлений в тексте
};

/**
 * @brief Структура для хранения статистических данных о тексте
 */
struct GlobalStats {
  int sentences_total;   // количество предложений в тексте
  int words_total;       // количество слов в тексте
  int members_total;     // количество членов предложения в тексте
  int subjects_total;    // количество подлежащий в тексте
  int predicates_total;  // количество сказуемых в тексте
  int definitions_total; // количество определений в тексте
  int additions_total;   // количество дополнений в тексте
  int adverbials_total;  // количество обстоятельств в тексте
  std::pair<std::string, int> top_subject;    // самое популярное подлежащее
  std::pair<std::string, int> top_predicate;  // самое популярное сказуемое
  std::pair<std::string, int> top_definition; // самое популярное определение
  std::pair<std::string, int> top_addition;   // самое популярное дополнение
  std::pair<std::string, int> top_adverbial;  // самое популярное обстоятельство
};

#endif // !STATISTICS_H
