#include "../../src/back/simple_tokenizer.h"
#include <cstddef>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Класс для тестирования protected и public методов SimpleTokenizer
 *
 * Этот класс делает protected методы публичными для доступа в тестах.
 */
class TestableTokenizer : public SimpleTokenizer {
public:
  using SimpleTokenizer::SimpleTokenizer;

  using SimpleTokenizer::find_token_in_text;
  using SimpleTokenizer::find_token_in_vocab;
  using SimpleTokenizer::split_text_into_tokens;
};

/**
 * @brief Вспомогательный класс для тестирования объекта токенизатора
 */
class TokenizerTest : public ::testing::Test {
protected:
  std::unique_ptr<TestableTokenizer> tokenizer;
  std::string test_vocab_path = "test_vocab.txt";

  void SetUp() override {
    createTestVocab();
    tokenizer = std::make_unique<TestableTokenizer>(test_vocab_path);
  }

  void createTestVocab() {
    const std::string path = test_vocab_path;
    std::ofstream vocab(path);

    vocab << "[PAD]\n"; // ID 0
    vocab << "[UNK]\n"; // ID 1
    vocab << "[CLS]\n"; // ID 2
    vocab << "[SEP]\n"; // ID 3
    vocab << "при\n";   // ID 4
    vocab << "##вет\n"; // ID 5
    vocab << ",\n";     // ID 6
    vocab << "мир\n";   // ID 7
    vocab << "как\n";   // ID 8
    vocab << "дела\n";  // ID 9
    vocab << "?\n";     // ID 10
    vocab << "!\n";     // ID 11

    vocab.close();
  }

  void TearDown() override {
    tokenizer.reset();
    std::remove(test_vocab_path.c_str());
  }
};

// тест определения id токена по словарю
TEST_F(TokenizerTest, FindTokenInVocab) {
  EXPECT_EQ(0, tokenizer->find_token_in_vocab("[PAD]"));
  EXPECT_EQ(1, tokenizer->find_token_in_vocab("[UNK]"));
  EXPECT_EQ(2, tokenizer->find_token_in_vocab("[CLS]"));
  EXPECT_EQ(3, tokenizer->find_token_in_vocab("[SEP]"));
  EXPECT_EQ(4, tokenizer->find_token_in_vocab("при"));
  EXPECT_EQ(5, tokenizer->find_token_in_vocab("##вет"));
  EXPECT_EQ(6, tokenizer->find_token_in_vocab(","));
  EXPECT_EQ(7, tokenizer->find_token_in_vocab("мир"));
  EXPECT_EQ(11, tokenizer->find_token_in_vocab("!"));
}

// тест разибиения текста на токены
TEST_F(TokenizerTest, SplitTextIntoTokens) {
  const std::string text = "Привет, мир! Как дела?";
  std::vector<std::string> tokens = tokenizer->split_text_into_tokens(text);

  EXPECT_EQ("при", tokens[0]);
  EXPECT_EQ("##вет", tokens[1]);
  EXPECT_EQ(",", tokens[2]);
  EXPECT_EQ("мир", tokens[3]);
  EXPECT_EQ("!", tokens[4]);
  EXPECT_EQ("как", tokens[5]);
  EXPECT_EQ("дела", tokens[6]);
  EXPECT_EQ("?", tokens[7]);
}

// тест определения начальной и конечной позиций токена в тексте
TEST_F(TokenizerTest, FindTokenInText) {
  const std::string text = "Привет, мир! Как дела?";

  std::pair<size_t, size_t> result1 =
      tokenizer->find_token_in_text(text, "при", 0);
  std::pair<size_t, size_t> result2 =
      tokenizer->find_token_in_text(text, "##ет", 0);
  std::pair<size_t, size_t> result3 =
      tokenizer->find_token_in_text(text, ",", 0);
  std::pair<size_t, size_t> result4 =
      tokenizer->find_token_in_text(text, "как", 0);

  std::pair<size_t, size_t> correct_result1 =
      std::make_pair<size_t, size_t>(0, 2);
  std::pair<size_t, size_t> correct_result2 =
      std::make_pair<size_t, size_t>(3, 5);
  std::pair<size_t, size_t> correct_result3 =
      std::make_pair<size_t, size_t>(6, 6);
  std::pair<size_t, size_t> correct_result4 =
      std::make_pair<size_t, size_t>(13, 15);

  EXPECT_EQ(correct_result1, result1);
  EXPECT_EQ(correct_result2, result2);
  EXPECT_EQ(correct_result3, result3);
  EXPECT_EQ(correct_result4, result4);
}

// тест энкодера
TEST_F(TokenizerTest, Encode) {
  const std::string text = "Привет, мир! Как дела?";
  SimpleTokenizer::EncodingResult res = tokenizer->encode(text);

  EXPECT_EQ(128, res.input_ids.size());
  EXPECT_EQ(128, res.tokens.size());
  EXPECT_EQ(128, res.attention_mask.size());
  EXPECT_EQ(4, res.word_ids.size());

  EXPECT_EQ(4, res.input_ids[0]);
  EXPECT_EQ(5, res.input_ids[1]);
  EXPECT_EQ(6, res.input_ids[2]);
  EXPECT_EQ(7, res.input_ids[3]);
  EXPECT_EQ(11, res.input_ids[4]);
  EXPECT_EQ(8, res.input_ids[5]);
  EXPECT_EQ(9, res.input_ids[6]);
  EXPECT_EQ(10, res.input_ids[7]);
  EXPECT_EQ(1, res.input_ids[8]);
  EXPECT_EQ(1, res.input_ids[127]);

  EXPECT_EQ("при", res.tokens[0]);
  EXPECT_EQ("##вет", res.tokens[1]);
  EXPECT_EQ(",", res.tokens[2]);
  EXPECT_EQ("мир", res.tokens[3]);
  EXPECT_EQ("!", res.tokens[4]);
  EXPECT_EQ("как", res.tokens[5]);
  EXPECT_EQ("дела", res.tokens[6]);
  EXPECT_EQ("?", res.tokens[7]);
  EXPECT_EQ("[PAD]", res.tokens[8]);
  EXPECT_EQ("[PAD]", res.tokens[127]);

  EXPECT_EQ(1, res.attention_mask[0]);
  EXPECT_EQ(1, res.attention_mask[2]);
  EXPECT_EQ(1, res.attention_mask[3]);
  EXPECT_EQ(1, res.attention_mask[4]);
  EXPECT_EQ(1, res.attention_mask[5]);
  EXPECT_EQ(1, res.attention_mask[6]);
  EXPECT_EQ(1, res.attention_mask[7]);
  EXPECT_EQ(0, res.attention_mask[8]);
  EXPECT_EQ(0, res.attention_mask[127]);
}
