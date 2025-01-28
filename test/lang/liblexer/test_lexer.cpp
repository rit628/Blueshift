#include "fixtures/lexer_test.hpp"
#include "test_macros.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace BlsLang {

    // Identifier Tests
    GROUP_TEST_F(LexerTest, IdentifierTests, Alphabetic) {
        std::string test_str = R"(someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "someText", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, Alphanumeric) {
        std::string test_str = R"(some1Text2)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "some1Text2", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingDigit) {
        std::string test_str = R"(1someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "1someText", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingHyphen) {
        std::string test_str = R"(-someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "-someText", 0, 0, 0),  
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    // Numeric Tests
    GROUP_TEST_F(LexerTest, NumericTests, IntegerLiteral) {
        std::string test_str = R"(123)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "123", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, NumericTests, NegativeInteger) {
        std::string test_str = R"(-456)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "-456", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // Decimal Tests
    GROUP_TEST_F(LexerTest, NumericTests, DecimalLiteral) {
        std::string test_str = R"(12.34)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "12.34", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // String Tests
    GROUP_TEST_F(LexerTest, StringTests, ValidString) {
        std::string test_str = R"("This is a string")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "This is a string", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, StringWithEscapeCharacters) {
        std::string test_str = R"("String with \n escape")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "String with \\n escape", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // Operator Tests
    GROUP_TEST_F(LexerTest, OperatorTests, SimpleOperator) {
        std::string test_str = R"(+)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "+", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, CompoundOperator) {
        std::string test_str = R"(<=)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "<=", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // Comment Tests
    GROUP_TEST_F(LexerTest, CommentTests, SingleLineComment) {
        std::string test_str = R"(// This is a comment)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "// This is a comment", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, CommentTests, MultiLineComment) {
        std::string test_str = R"(/* Multi-line 
                                    comment */)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "/* Multi-line \n                                    comment */", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // Edge Cases
    GROUP_TEST_F(LexerTest, EdgeTests, WhitespaceHandling) {
        std::string test_str = R"(   identifier   )";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "identifier", 3, 0, 3),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, MixedTokens) {
        std::string test_str = R"(123 + 456)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "123", 0, 0, 0),
            Token(Token::Type::OPERATOR, "+", 4, 0, 4),
            Token(Token::Type::INTEGER, "456", 6, 0, 6),
        };
        TEST_LEX(test_str, exp_tokens);
    }

}