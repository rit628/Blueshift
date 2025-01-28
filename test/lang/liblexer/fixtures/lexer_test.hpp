#pragma once
#include "lexer.hpp"
#include <gtest/gtest.h>
#include <string>

namespace BlsLang {
    class LexerTest : public testing::Test {
        public:
            void TEST_LEX(std::string sampleSource, std::vector<Token> expectedTokens, bool success = true) {
                auto tokens = lexer.lex(sampleSource);
                if (success) {
                    EXPECT_EQ(tokens, expectedTokens);
                }
                else {
                    EXPECT_NE(tokens,  expectedTokens);
                }
            }

        private:
            Lexer lexer;
    };
}