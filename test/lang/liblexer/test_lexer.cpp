#include "fixtures/lexer_test.hpp"
#include "test_macros.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(LexerTest, IdentifierTests, Alphabetic) {
        std::string test_str = R"(someText)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::IDENTIFIER, "someText", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, Alphanumeric) {
        std::string test_str = R"(some1Text2)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::IDENTIFIER, "some1Text2", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingDigit) {
        std::string test_str = R"(1someText)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::IDENTIFIER, "1someText", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingHyphen) {
        std::string test_str = R"(-someText)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::IDENTIFIER, "-someText", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, NumericTests, Integer) {
        std::string test_str = R"(7)";
        std::vector<BlsLang::Token> exp_tokens {
            BlsLang::Token(BlsLang::Token::Type::INTEGER, "7", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

}
