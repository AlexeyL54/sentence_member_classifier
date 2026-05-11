#include "statistics.hpp"
#include "bert_onnx_inference.hpp"
#include <QDebug>
#include <QHash>
#include <QMap>
#include <QString>

/**
 * @brief Подсчитывает количество слов в строке предложения.
 * @param text Исходный текст предложения.
 * @return Количество непрерывных последовательностей непробельных символов.
 */
static int countWordsInText(const QString &text) {
  int wordCount = 0;
  bool insideWord = false;
  for (int i = 0; i < text.length(); ++i) {
    if (text[i].isSpace()) {
      insideWord = false;
    } else if (!insideWord) {
      ++wordCount;
      insideWord = true;
    }
  }
  return wordCount;
}

/**
 * @brief Возвращает наиболее частое слово для категории.
 * @param freq Частотный словарь (слово -> количество вхождений).
 * @return Пара (слово, количество). При равенстве частоты выбирается
 *         лексикографически меньшее слово.
 */
static std::pair<QString, int> pickTopWord(const QHash<QString, int> &freq) {
  std::pair<QString, int> best{"", 0};
  for (auto it = freq.constBegin(); it != freq.constEnd(); ++it) {
    const QString &word = it.key();
    int count = it.value();
    if (count > best.second || (count == best.second && word < best.first)) {
      best = {word, count};
    }
  }
  return best;
}

/**
 * @brief Возвращает самый частый фрагмент по категории или значение «нет», если
 * выделить нельзя.
 * @param freq Частотный словарь (слово -> количество вхождений).
 * @return Если категория пуста или максимальная частота равна 1 (все члены этой
 * категории встречаются по одному разу) — возвращает пару ("-", 0). Иначе —
 * самую частую пару.
 */
static std::pair<QString, int>
pickTopWordOrNone(const QHash<QString, int> &freq) {
  const std::pair<QString, int> best = pickTopWord(freq);
  if (freq.isEmpty() || best.second <= 1)
    return {"-", 0};
  return best;
}

/**
 * @brief Добавляет контекст предложения, если для этого номера ещё нет.
 * @param[in,out] contexts Вектор пар (номер предложения, текст предложения).
 * @param sentence_number Номер предложения (1-based).
 * @param text Текст предложения.
 */
static void
pushSentenceContextIfMissing(std::vector<std::pair<int, QString>> &contexts,
                             int sentence_number, const QString &text) {
  for (const auto &c : contexts) {
    if (c.first == sentence_number)
      return;
  }
  contexts.push_back({sentence_number, text});
}

namespace CategoryRu {
const QString SUBJECT = "подлежащее";
const QString PREDICATE = "сказуемое";
const QString DEFINITION = "определение";
const QString ADDITION = "дополнение";
const QString ADVERBIAL = "обстоятельство";
const QString OTHER = "другое";
} // namespace CategoryRu

/**
 * @brief Увеличивает соответствующий счётчик в глобальной статистике в
 * зависимости от категории.
 * @param[in,out] stats Глобальная статистика.
 * @param categoryRu Название категории на русском языке.
 */
static void incrementCategoryTotal(GlobalStats &stats,
                                   const QString &categoryRu) {
  if (categoryRu == CategoryRu::SUBJECT)
    stats.subjects_total += 1;
  else if (categoryRu == CategoryRu::PREDICATE)
    stats.predicates_total += 1;
  else if (categoryRu == CategoryRu::DEFINITION)
    stats.definitions_total += 1;
  else if (categoryRu == CategoryRu::ADDITION)
    stats.additions_total += 1;
  else if (categoryRu == CategoryRu::ADVERBIAL)
    stats.adverbials_total += 1;
  else if (categoryRu == CategoryRu::OTHER)
    stats.others_total += 1;
}

/**
 * @brief Структура для хранения частот слов по категориям.
 *        Используется для вычисления самых частых членов предложения.
 */
struct CategoryWordFreqs {
  QHash<QString, int> subject;    ///< Частоты подлежащих
  QHash<QString, int> predicate;  ///< Частоты сказуемых
  QHash<QString, int> definition; ///< Частоты определений
  QHash<QString, int> addition;   ///< Частоты дополнений
  QHash<QString, int> adverbial;  ///< Частоты обстоятельств
  QHash<QString, int> other;      ///< Частоты других членов предложения

  /**
   * @brief Увеличивает счётчик для указанной категории.
   * @param categoryRu Название категории на русском языке.
   * @param word Слово (член предложения) в нижнем регистре.
   */
  void increment(const QString &categoryRu, const QString &word) {
    if (categoryRu == CategoryRu::SUBJECT)
      subject[word] += 1;
    else if (categoryRu == CategoryRu::PREDICATE)
      predicate[word] += 1;
    else if (categoryRu == CategoryRu::DEFINITION)
      definition[word] += 1;
    else if (categoryRu == CategoryRu::ADDITION)
      addition[word] += 1;
    else if (categoryRu == CategoryRu::ADVERBIAL)
      adverbial[word] += 1;
    else if (categoryRu == CategoryRu::OTHER)
      other[word] += 1;
  }
};

/**
 * @brief Заполняет поля top_* в глобальной статистике из накопленных частот.
 * @param[out] stats Глобальная статистика.
 * @param freqs Накопленные частоты слов по категориям.
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
 * @brief Учитывает одну сущность (член предложения) в глобальной статистике и
 * частотах по категориям.
 * @param[in,out] stats Глобальная статистика.
 * @param[in,out] freqs Частоты слов по категориям.
 * @param entity Обрабатываемая сущность (член предложения).
 */
static void accumulateEntityForGlobalStats(GlobalStats &stats,
                                           CategoryWordFreqs &freqs,
                                           const Entity &entity) {
  QString word = QString::fromStdString(entity.text);
  QString categoryRu = QString::fromStdString(entity.type_ru);
  if (word.isEmpty() || categoryRu.isEmpty()) {
    return;
  }

  QString wordKey = word.toLower();

  stats.members_total += 1;
  incrementCategoryTotal(stats, categoryRu);
  freqs.increment(categoryRu, wordKey);
}

/**
 * @brief Строит глобальную статистику по результатам анализа предложений.
 * @param analysis_results Вектор результатов анализа предложений.
 * @return Заполненная структура GlobalStats.
 */
GlobalStats
build_global_stats(const std::vector<SentenceResult> &analysis_results) {
  GlobalStats stats{};
  stats.sentences_total = static_cast<int>(analysis_results.size());

  CategoryWordFreqs freqs;

  for (const SentenceResult &sentenceResult : analysis_results) {
    stats.words_total +=
        countWordsInText(QString::fromStdString(sentenceResult.text));

    for (const Entity &entity : sentenceResult.entities) {
      accumulateEntityForGlobalStats(stats, freqs, entity);
    }
  }

  fillTopStats(stats, freqs);
  return stats;
}

/**
 * @brief Возвращает ключ для группировки слова (нижний регистр).
 * @param word Исходное слово.
 * @return Слово в нижнем регистре.
 */
static QString normalizedWordKey(const QString &word) { return word.toLower(); }

/**
 * @brief Добавляет вхождение сущности в агрегат по ключу (категория, слово).
 * @param[in,out] itemsByKey Карта, где ключ — пара (категория, слово в нижнем
 * регистре), значение — агрегированный SearchItem.
 * @param sentence_number Номер предложения (1-based).
 * @param sentenceText Текст предложения.
 * @param entity Обрабатываемая сущность.
 */
static void mergeEntityIntoSearchItems(
    std::map<std::pair<QString, QString>, SearchItem> &itemsByKey,
    int sentence_number, const QString &sentenceText, const Entity &entity) {

  QString word = QString::fromStdString(entity.text);
  QString categoryRu = QString::fromStdString(entity.type_ru);
  if (word.isEmpty() || categoryRu.isEmpty()) {
    return;
  }

  QString wordKey = normalizedWordKey(word);
  auto key = std::make_pair(categoryRu, wordKey);
  SearchItem &item = itemsByKey[key];

  if (item.amount == 0) {
    item.text = word;
    item.type = categoryRu;
  }

  item.amount += 1;
  pushSentenceContextIfMissing(item.sentences, sentence_number, sentenceText);
}

/**
 * @brief Строит агрегированный список элементов для страницы поиска.
 * @param analysis_results Вектор результатов анализа предложений.
 * @return Вектор структур SearchItem.
 */
std::vector<SearchItem>
build_search_items(const std::vector<SentenceResult> &analysis_results) {
  std::map<std::pair<QString, QString>, SearchItem> itemsByKey;

  for (size_t si = 0; si < analysis_results.size(); ++si) {
    const SentenceResult &sentenceResult = analysis_results[si];
    int sentence_number = static_cast<int>(si) + 1;
    QString sentenceText = QString::fromStdString(sentenceResult.text);

    for (const Entity &entity : sentenceResult.entities) {
      mergeEntityIntoSearchItems(itemsByKey, sentence_number, sentenceText,
                                 entity);
    }
  }

  std::vector<SearchItem> items;
  items.reserve(itemsByKey.size());
  for (auto &[key, item] : itemsByKey) {
    Q_UNUSED(key);
    items.push_back(std::move(item));
  }
  return items;
}
