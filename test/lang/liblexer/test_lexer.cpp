#include "fixtures/lexer_test.hpp"
#include "test_macros.hpp"
#include "token.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
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

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingUnderscore) {
        std::string test_str = R"(_someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "_someText", 0, 0, 0),  
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
            Token(Token::Type::STRING, "\"This is a string\"", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, StringWithEscapeCharacters) {
        std::string test_str = R"("String with \n escape")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"String with \\n escape\"", 0, 0, 0),
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

    GROUP_TEST_F(LexerTest, EdgeTests, UnclosedComment) {
        std::string test_str = R"(/* Multiline comment unclosed)";
        std::vector<Token> exp_tokens {};
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
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

    GROUP_TEST_F(LexerTest, EdgeTests, EmptyInput) {
        std::string test_str = R"()";
        std::vector<Token> exp_tokens {};
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, AllWhitespace) {
        std::string test_str = "   \t\n   ";
        std::vector<Token> exp_tokens {};
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, MixedWhitespaceAndTokens) {
        std::string test_str = "  id1\t+ \n   id2  ";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "id1", 2, 0, 2),
            Token(Token::Type::OPERATOR, "+", 6, 0, 6),
            Token(Token::Type::IDENTIFIER, "id2", 12, 1, 3),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, StringUnterminated) {
        std::string test_str = R"("unterminated string)";
        std::vector<Token> exp_tokens {};
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, InvalidNumericLiteral) {
        std::string test_str = R"(12.)";
        std::vector<Token> exp_tokens {};
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, MultiLineStringWithEscapes) {
        std::string test_str = R"("This is a multi-line\nstring with escapes")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"This is a multi-line\\nstring with escapes\"", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, CompoundOperatorsAndIdentifiers) {
        std::string test_str = R"(id1++id2--)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "id1", 0, 0, 0),
            Token(Token::Type::OPERATOR, "++", 3, 0, 3),
            Token(Token::Type::IDENTIFIER, "id2", 5, 0, 5),
            Token(Token::Type::OPERATOR, "--", 8, 0, 8),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, NumericWithLeadingZeros) {
        std::string test_str = R"(00123 0.045)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "00123", 0, 0, 0),
            Token(Token::Type::DECIMAL, "0.045", 6, 0, 6),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, ComplexStringWithSpecialCharacters) {
        std::string test_str = R"("Special characters: ~!@#$%^&*()_+{}|:<>?")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"Special characters: ~!@#$%^&*()_+{}|:<>?\"", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, NestedComments) {
        std::string test_str = R"(/* Outer /* Inner */ Still outer */)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "/* Outer /* Inner */", 0, 0, 0),
            Token(Token::Type::IDENTIFIER, "Still", 21, 0, 21),
            Token(Token::Type::IDENTIFIER, "outer", 27, 0, 27),
            Token(Token::Type::OPERATOR, "*", 33, 0, 33),
            Token(Token::Type::OPERATOR, "/", 34, 0, 34)
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, NestedCommentsUnclosed) {
        std::string test_str = R"(/* Outer /* Inner Still outer */)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "/* Outer /* Inner Still outer */", 0, 0, 0)
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, EscapedQuotesInString) {
        std::string test_str = R"("This is a \"quoted\" string")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"This is a \\\"quoted\\\" string\"", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, EmptyStringLiteral) {
        std::string test_str = R"("")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, MixedIdentifiersAndNumbers) {
        std::string test_str = R"(id123 456id)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "id123", 0, 0, 0),
            Token(Token::Type::INTEGER, "456", 6, 0, 6),
            Token(Token::Type::IDENTIFIER, "id", 9, 0, 9),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, EscapedNewlinesInString) {
        std::string test_str = R"("String with newline \n and tab \t")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"String with newline \\n and tab \\t\"", 0, 0, 0),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, EdgeTests, OnlyOperators) {
        std::string test_str = R"(++++----****)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "++", 0, 0, 0),
            Token(Token::Type::OPERATOR, "++", 2, 0, 2),
            Token(Token::Type::OPERATOR, "--", 4, 0, 4),
            Token(Token::Type::OPERATOR, "--", 6, 0, 6),
            Token(Token::Type::OPERATOR, "*", 8, 0, 8),
            Token(Token::Type::OPERATOR, "*", 9, 0, 9),
            Token(Token::Type::OPERATOR, "*", 10, 0, 10),
            Token(Token::Type::OPERATOR, "*", 11, 0, 11)
        };
        TEST_LEX(test_str, exp_tokens);
    }

}