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
                    for (int i = 0; i < tokens.size(); i++) {
                        EXPECT_EQ(tokens[i].getType(), expectedTokens[i].getType());
                        EXPECT_EQ(tokens[i].getLiteral(), expectedTokens[i].getLiteral());
                        EXPECT_EQ(tokens[i].getAbsIdx(), expectedTokens[i].getAbsIdx());
                        EXPECT_EQ(tokens[i].getLineNum(), expectedTokens[i].getLineNum());
                        EXPECT_EQ(tokens[i].getColNum(), expectedTokens[i].getColNum());
                    }
                }
                else {
                    EXPECT_NE(tokens,  expectedTokens);
                }
            }

        private:
            Lexer lexer;
    };
}