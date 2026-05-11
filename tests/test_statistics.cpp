#include "../src/back/bert_onnx_inference.hpp"
#include "../src/back/statistics.hpp"
#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

TEST(StatisticsTest, BuildGlobalStats_EmptyInput) {
  std::vector<SentenceResult> analysis_results = {};
  GlobalStats stats = build_global_stats(analysis_results);

  EXPECT_EQ(0, stats.sentences_total);
  EXPECT_EQ(0, stats.words_total);
  EXPECT_EQ(0, stats.members_total);
  EXPECT_EQ("-", stats.top_subject.first);
  EXPECT_EQ(0, stats.top_subject.second);
}

TEST(StatisticsTest, BuildGlobalStats_SingleSentence_NoEntities) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result;
  result.text = "Привет мир!";
  result.entities = {};
  analysis_results.push_back(result);
  GlobalStats stats = build_global_stats(analysis_results);

  EXPECT_EQ(1, stats.sentences_total);
  EXPECT_EQ(2, stats.words_total);
  EXPECT_EQ(0, stats.members_total);
}

TEST(StatisticsTest, BuildGlobalStats_WordCounting) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result;

  result.text = "Один два три четыре пять";
  result.entities = {};
  analysis_results.push_back(result);
  GlobalStats stats = build_global_stats(analysis_results);

  EXPECT_EQ(1, stats.sentences_total);
  EXPECT_EQ(5, stats.words_total);
}

TEST(StatisticsTest, BuildGlobalStats_CategoryCounts) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result;
  result.text = "Мама мыла раму";

  Entity subject_entity;
  Entity predicate_entity;
  Entity addition_entity;

  subject_entity.text = "Мама";
  subject_entity.type_ru = "подлежащее";
  predicate_entity.text = "мыла";
  predicate_entity.type_ru = "сказуемое";
  addition_entity.text = "раму";
  addition_entity.type_ru = "дополнение";

  result.entities = {subject_entity, predicate_entity, addition_entity};
  analysis_results.push_back(result);
  GlobalStats stats = build_global_stats(analysis_results);

  EXPECT_EQ(1, stats.sentences_total);
  EXPECT_EQ(3, stats.words_total);
  EXPECT_EQ(3, stats.members_total);
  EXPECT_EQ(1, stats.subjects_total);
  EXPECT_EQ(1, stats.predicates_total);
  EXPECT_EQ(1, stats.additions_total);
}

TEST(StatisticsTest, BuildGlobalStats_TopWord_Selection) {
  std::vector<SentenceResult> analysis_results = {};
  for (int i = 0; i < 3; i++) {
    SentenceResult result;
    result.text = "Кот спит";
    Entity subject_entity;
    subject_entity.text = "Кот";
    subject_entity.type_ru = "подлежащее";
    Entity predicate_entity;
    predicate_entity.text = "спит";
    predicate_entity.type_ru = "сказуемое";
    result.entities = {subject_entity, predicate_entity};
    analysis_results.push_back(result);
  }
  SentenceResult result2;
  result2.text = "Собака бежит";
  Entity dog_entity;
  dog_entity.text = "Собака";
  dog_entity.type_ru = "подлежащее";
  Entity run_entity;
  run_entity.text = "бежит";
  run_entity.type_ru = "сказуемое";
  result2.entities = {dog_entity, run_entity};
  analysis_results.push_back(result2);
  GlobalStats stats = build_global_stats(analysis_results);

  EXPECT_EQ(4, stats.sentences_total);
  EXPECT_EQ(4, stats.subjects_total);
  EXPECT_EQ("кот", stats.top_subject.first);
  EXPECT_EQ(3, stats.top_subject.second);
}

TEST(StatisticsTest, BuildGlobalStats_TopWordOrNone_WhenMaxFrequencyIsOne) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result;
  result.text = "Уникальное слово";
  Entity unique_entity;
  unique_entity.text = "Уникальное";
  unique_entity.type_ru = "подлежащее";
  result.entities = {unique_entity};
  analysis_results.push_back(result);
  GlobalStats stats = build_global_stats(analysis_results);

  EXPECT_EQ("-", stats.top_subject.first);
  EXPECT_EQ(0, stats.top_subject.second);
}

TEST(StatisticsTest, BuildSearchItems_EmptyInput) {
  std::vector<SentenceResult> analysis_results = {};
  std::vector<SearchItem> items = build_search_items(analysis_results);
  EXPECT_TRUE(items.empty());
}

TEST(StatisticsTest, BuildSearchItems_SingleEntity) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result;
  result.text = "Мама мыла раму";
  Entity entity;
  entity.text = "Мама";
  entity.type_ru = "подлежащее";
  result.entities = {entity};
  analysis_results.push_back(result);
  std::vector<SearchItem> items = build_search_items(analysis_results);

  EXPECT_EQ(1, items.size());
  EXPECT_EQ("Мама", items[0].text);
  EXPECT_EQ("подлежащее", items[0].type);
  EXPECT_EQ(1, items[0].amount);
  EXPECT_EQ(1, items[0].sentences.size());
  EXPECT_EQ(1, items[0].sentences[0].first);
  EXPECT_EQ("Мама мыла раму", items[0].sentences[0].second);
}

TEST(StatisticsTest, BuildSearchItems_MultipleSentences_SameEntity) {
  std::vector<SentenceResult> analysis_results = {};
  for (int i = 0; i < 3; i++) {
    SentenceResult result;
    result.text = "Кот спит";
    Entity entity;
    entity.text = "Кот";
    entity.type_ru = "подлежащее";
    result.entities = {entity};
    analysis_results.push_back(result);
  }
  std::vector<SearchItem> items = build_search_items(analysis_results);

  EXPECT_EQ(1, items.size());
  EXPECT_EQ("Кот", items[0].text);
  EXPECT_EQ(3, items[0].amount);
  EXPECT_EQ(3, items[0].sentences.size());
}

TEST(StatisticsTest, BuildSearchItems_DifferentEntities) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result;
  result.text = "Мама мыла раму";
  Entity subject;
  subject.text = "Мама";
  subject.type_ru = "подлежащее";
  Entity predicate;
  predicate.text = "мыла";
  predicate.type_ru = "сказуемое";
  Entity addition;
  addition.text = "раму";
  addition.type_ru = "дополнение";
  result.entities = {subject, predicate, addition};
  analysis_results.push_back(result);
  std::vector<SearchItem> items = build_search_items(analysis_results);

  EXPECT_EQ(3, items.size());
}

TEST(StatisticsTest, BuildSearchItems_CaseInsensitive_Aggregation) {
  std::vector<SentenceResult> analysis_results = {};
  SentenceResult result1;
  result1.text = "КОТ";
  Entity cat1;
  cat1.text = "КОТ";
  cat1.type_ru = "подлежащее";
  result1.entities = {cat1};
  analysis_results.push_back(result1);
  SentenceResult result2;
  result2.text = "Кот";
  Entity cat2;
  cat2.text = "Кот";
  cat2.type_ru = "подлежащее";
  result2.entities = {cat2};
  analysis_results.push_back(result2);
  std::vector<SearchItem> items = build_search_items(analysis_results);

  EXPECT_EQ(1, items.size());
  EXPECT_EQ(2, items[0].amount);
  EXPECT_EQ("КОТ", items[0].text);
}
