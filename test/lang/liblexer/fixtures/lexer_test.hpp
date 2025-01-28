#include "lexer.hpp"
#include <gtest/gtest.h>
#include <string>

namespace BlsLang {
    class LexerTest : public testing::Test {
        public:
            void TEST_LEX(std::string sampleSource, std::vector<Token> expectedTokens) {
                auto tokens = lexer.lex(sampleSource);
                EXPECT_EQ(tokens, expectedTokens);
            }

        private:
            Lexer lexer;
    };
}