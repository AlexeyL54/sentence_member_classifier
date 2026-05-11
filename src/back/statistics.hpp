#ifndef STATISTICS_H
#define STATISTICS_H

#include "bert_onnx_inference.hpp"
#include <QString>
#include <utility>
#include <vector>

/**
 * @brief Элемент агрегированного списка членов предложения для страницы поиска.
 */
struct SearchItem {
  QString text;   // Текст члена предложения
  QString type;   // Тип члена предложения (подлежащее, сказуемое и т.д.)
  int amount = 0; // Количество вхождений в тексте
  std::vector<std::pair<int, QString>>
      sentences; // Пары (номер предложения, полный текст предложения)
};

/**
 * @brief Глобальная статистика по обработанному тексту (экран анализа).
 */
struct GlobalStats {
  int sentences_total = 0;   // Общее количество предложений
  int words_total = 0;       // Общее количество слов
  int members_total = 0;     // Общее количество членов предложения
  int subjects_total = 0;    // Количество подлежащих
  int predicates_total = 0;  // Количество сказуемых
  int definitions_total = 0; // Количество определений
  int additions_total = 0;   // Количество дополнений
  int adverbials_total = 0;  // Количество обстоятельств
  int others_total = 0;      // Количество других членов предложения

  std::pair<QString, int>
      top_subject; // Самое частое подлежащее (слово, количество)
  std::pair<QString, int>
      top_predicate; // Самое частое сказуемое (слово, количество)
  std::pair<QString, int>
      top_definition; // Самое частое определение (слово, количество)
  std::pair<QString, int>
      top_addition; // Самое частое дополнение (слово, количество)
  std::pair<QString, int>
      top_adverbial; // Самое частое обстоятельство (слово, количество)
  std::pair<QString, int>
      top_other; // Самый частый другой член предложения (слово, количество)
};

/**
 * @brief Строит глобальную статистику по результатам анализа предложений.
 *
 * Подсчитывает общее количество предложений, слов, членов предложения по
 * категориям, а также определяет самые частые члены предложения для каждой
 * категории.
 *
 * @param analysis_results Вектор результатов анализа предложений из
 * extract_sentence_parts().
 * @return Заполненная структура GlobalStats со всей статистикой.
 */
GlobalStats
build_global_stats(const std::vector<SentenceResult> &analysis_results);

/**
 * @brief Строит агрегированный список элементов для страницы поиска.
 *
 * Группирует члены предложения по типу и тексту (в нижнем регистре),
 * собирая для каждого уникального члена предложения количество вхождений
 * и контексты предложений, в которых он встречается.
 *
 * @param analysis_results Вектор результатов анализа предложений из
 * extract_sentence_parts().
 * @return Вектор структур SearchItem, отсортированный по типу и тексту.
 */
std::vector<SearchItem>
build_search_items(const std::vector<SentenceResult> &analysis_results);

#endif // !STATISTICS_H
