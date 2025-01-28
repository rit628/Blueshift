#include "fixtures/lexer_test.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace BlsLang {

    TEST_F(LexerTest, BasicIdentifier) {
        std::string test_str = R"(basic)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::IDENTIFIER, "basic", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    TEST_F(LexerTest, BasicIdentifierWithNumbers) {
        std::string test_str = R"(basic123)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::IDENTIFIER, "basic123", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

}
