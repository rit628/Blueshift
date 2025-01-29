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

    //
    // ===============
    // Integer Tests
    // ===============
    //

    // SingleDigit
    GROUP_TEST_F(LexerTest, NumericTests, SingleDigit) {
        std::string test_str = R"(1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "1", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // MultipleDigits
    GROUP_TEST_F(LexerTest, NumericTests, MultipleDigits) {
        std::string test_str = R"(12345)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "12345", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // TrailingZeros
    GROUP_TEST_F(LexerTest, NumericTests, TrailingZeros) {
        std::string test_str = R"(100000)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "100000", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // Zero
    GROUP_TEST_F(LexerTest, NumericTests, Zero) {
        std::string test_str = R"(0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // MiddleZero
    GROUP_TEST_F(LexerTest, NumericTests, MiddleZero) {
        std::string test_str = R"(102)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "102", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // LeadingZero
    GROUP_TEST_F(LexerTest, NumericTests, LeadingZero) {
        std::string test_str = R"(01)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "01", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // MultipleZeros
    GROUP_TEST_F(LexerTest, NumericTests, MultipleZeros) {
        std::string test_str = R"(00)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "00", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeZero
    GROUP_TEST_F(LexerTest, NumericTests, NegativeZero) {
        std::string test_str = R"(-0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "-0", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // PositiveDecimal
    GROUP_TEST_F(LexerTest, NumericTests, PositiveDecimal) {
        std::string test_str = R"(0.1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0.1", 0, 0, 0), // We'll still label it as INTEGER to fail
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // NegativeDecimal
    GROUP_TEST_F(LexerTest, NumericTests, NegativeDecimal) {
        std::string test_str = R"(-0.1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "-0.1", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }


    //
    // ===============
    // Decimal Tests
    // ===============
    //

    // MultipleDigitsDecimal
    GROUP_TEST_F(LexerTest, NumericTests, MultipleDigitsDecimal) {
        std::string test_str = R"(123.456)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "123.456", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // One
    GROUP_TEST_F(LexerTest, NumericTests, OneDecimal) {
        std::string test_str = R"(1.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "1.0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // ZeroDecimal
    GROUP_TEST_F(LexerTest, NumericTests, ZeroDecimal) {
        std::string test_str = R"(0.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "0.0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeOneDecimal
    GROUP_TEST_F(LexerTest, NumericTests, NegativeOneDecimal) {
        std::string test_str = R"(-1.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-1.0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // OneHundredAndTwentyThree
    GROUP_TEST_F(LexerTest, NumericTests, OneHundredAndTwentyThree) {
        std::string test_str = R"(123.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "123.0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeOneHundredAndTwentyThree
    GROUP_TEST_F(LexerTest, NumericTests, NegativeOneHundredAndTwentyThree) {
        std::string test_str = R"(-123.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-123.0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // FifteenHundredths
    GROUP_TEST_F(LexerTest, NumericTests, FifteenHundredths) {
        std::string test_str = R"(0.15)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "0.15", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeFifteenHundredths
    GROUP_TEST_F(LexerTest, NumericTests, NegativeFifteenHundredths) {
        std::string test_str = R"(-0.15)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-0.15", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeZeroDecimal
    GROUP_TEST_F(LexerTest, NumericTests, NegativeZeroDecimal2) {
        std::string test_str = R"(-0.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-0.0", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NoDecimal
    GROUP_TEST_F(LexerTest, NumericTests, NoDecimal) {
        std::string test_str = R"(1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "1", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // NoInteger
    GROUP_TEST_F(LexerTest, NumericTests, NoInteger) {
        std::string test_str = R"(.1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, ".1", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // NoIntegerTrailing0
    GROUP_TEST_F(LexerTest, NumericTests, NoIntegerTrailing0) {
        std::string test_str = R"(.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, ".0", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // IntegerWithTrailingDecimalPoint
    GROUP_TEST_F(LexerTest, NumericTests, IntegerWithTrailingDecimalPoint) {
        std::string test_str = R"(1.)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "1.", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // ZeroWithTrailingDecimalPoint
    GROUP_TEST_F(LexerTest, NumericTests, ZeroWithTrailingDecimalPoint) {
        std::string test_str = R"(0.)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "0.", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);

    }

    // LeadingZeroesDecimal
    GROUP_TEST_F(LexerTest, NumericTests, LeadingZeroesDecimal) {
        std::string test_str = R"(0123.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "0123.0", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeLeadingZeroesDecimal
    GROUP_TEST_F(LexerTest, NumericTests, NegativeLeadingZeroesDecimal) {
        std::string test_str = R"(-0123.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-0123.0", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeDecimalNoInteger
    GROUP_TEST_F(LexerTest, NumericTests, NegativeDecimalNoInteger) {
        std::string test_str = R"(-.15)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-.15", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // TrailingDecimal
    GROUP_TEST_F(LexerTest, NumericTests, TrailingDecimal2) {
        std::string test_str = R"(1.)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "1.", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // LeadingDecimal
    GROUP_TEST_F(LexerTest, NumericTests, LeadingDecimal2) {
        std::string test_str = R"(.5)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, ".5", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }


    //
    // ===============
    // String Tests
    // ===============
    //

    // EmptyString
    GROUP_TEST_F(LexerTest, StringTests, EmptyString) {
        std::string test_str = R"("")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NormalString
    GROUP_TEST_F(LexerTest, StringTests, NormalString) {
        std::string test_str = R"("Hello world!")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"Hello world!\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // AlphabeticString
    GROUP_TEST_F(LexerTest, StringTests, AlphabeticString) {
        std::string test_str = R"("abc")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"abc\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NewlineEscapeString
    GROUP_TEST_F(LexerTest, StringTests, NewlineEscapeString) {
        std::string test_str = R"("Hello,\nWorld")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"Hello,\\nWorld\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // EndsWithEscape
    GROUP_TEST_F(LexerTest, StringTests, EndsWithEscape) {
        std::string test_str = R"("test_string\n")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"test_string\\n\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // ProperQuoteEscape
    GROUP_TEST_F(LexerTest, StringTests, ProperQuoteEscape) {
        // "Hello \"Bob\" "
        std::string test_str = "\"Hello \\\"Bob\\\" \"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"Hello \\\"Bob\\\" \"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // MultipleEscapes
    GROUP_TEST_F(LexerTest, StringTests, MultipleEscapes) {
        std::string test_str = R"("test\nstring\nnew")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"test\\nstring\\nnew\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // EscapeSequences
    GROUP_TEST_F(LexerTest, StringTests, EscapeSequences) {
        // "\b\r\t\n\'\"\\"
        std::string test_str = "\"\\b\\r\\t\\n\\'\\\"\\\\\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\b\\r\\t\\n\\'\\\"\\\\\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // SpecialCharacters
    GROUP_TEST_F(LexerTest, StringTests, SpecialCharacters) {
        std::string test_str = R"("`~!@#$%^&*()-_=+[{]}|;:,<.>/? ")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"`~!@#$%^&*()-_=+[{]}|;:,<.>/? \"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // EscapedNestedQuote
    GROUP_TEST_F(LexerTest, StringTests, EscapedNestedQuote) {
        std::string test_str = R"("\"")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\\"\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // EscapedNestedString
    GROUP_TEST_F(LexerTest, StringTests, EscapedNestedString) {
        std::string test_str = R"("\"\"")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\\"\\\"\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // ActiveBackspace
    GROUP_TEST_F(LexerTest, StringTests, ActiveBackspace) {
        std::string test_str = "\"\b\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\b\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // ActiveTab
    GROUP_TEST_F(LexerTest, StringTests, ActiveTab) {
        std::string test_str = "\"\t\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\t\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // FormFeed
    GROUP_TEST_F(LexerTest, StringTests, FormFeed) {
        std::string test_str = "\"\f\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\f\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // VerticalTab
    GROUP_TEST_F(LexerTest, StringTests, VerticalTab) {
        std::string test_str = "\"\u000B\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\u000B\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // EscapedBackslash
    GROUP_TEST_F(LexerTest, StringTests, EscapedBackslash) {
        std::string test_str = R"("\\")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\\\\"", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NonEscapedBackslash
    GROUP_TEST_F(LexerTest, StringTests, NonEscapedBackslash) {
        std::string test_str = R"("\")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\\"", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens, false), std::runtime_error);
    }

    // LiteralEmptyString
    GROUP_TEST_F(LexerTest, StringTests, LiteralEmptyString) {
        // this is an empty input, no quotes
        std::string test_str = "";
        std::vector<Token> exp_tokens {
            // no tokens
        };
        // success == false
        TEST_LEX(test_str, exp_tokens);
    }

    // InvalidEscapeSequences
    GROUP_TEST_F(LexerTest, StringTests, InvalidEscapeSequences) {
        std::string test_str = "\"\\q\\w\\e\\1\\2\\`\\&\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\q\\w\\e\\1\\2\\`\\&\"", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // UnterminatedString
    GROUP_TEST_F(LexerTest, StringTests, UnterminatedString) {
        std::string test_str = R"(")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // NestedString
    GROUP_TEST_F(LexerTest, StringTests, NestedString) {
        std::string test_str = R"("" "")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"\"\"", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // NestedQuote
    GROUP_TEST_F(LexerTest, StringTests, NestedQuote) {
        std::string test_str = R"("""")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"\"", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // StringLiteralBrokenByNewline
    GROUP_TEST_F(LexerTest, StringTests, StringLiteralBrokenByNewline) {
        std::string test_str = "\"\n\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\n\"", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // StringLiteralBrokenByCarriageReturn
    GROUP_TEST_F(LexerTest, StringTests, StringLiteralBrokenByCarriageReturn) {
        std::string test_str = "\"\r\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\r\"", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // Unterminated
    GROUP_TEST_F(LexerTest, StringTests, Unterminated2) {
        std::string test_str = R"("unterminated)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"unterminated", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // InvalidEscape
    GROUP_TEST_F(LexerTest, StringTests, InvalidEscape) {
        std::string test_str = R"("invalid\escape")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"invalid\\escape\"", 0, 0, 0),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // NewlineInString
    GROUP_TEST_F(LexerTest, StringTests, NewlineInString) {
        std::string test_str = " \"juice\nwhale\" ";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"juice\nwhale\"", 1, 0, 1),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }

    // NewlineInStringEnd
    GROUP_TEST_F(LexerTest, StringTests, NewlineInStringEnd) {
        std::string test_str = " \"juice whale\n\" ";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"juice whale\n\"", 1, 0, 1),
        };
        // success == false
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), std::runtime_error);
    }


    //
    // ===============
    // Operator Tests
    // ===============
    //

    // SingleCharacterOperator
    GROUP_TEST_F(LexerTest, OperatorTests, SingleCharacterOperator) {
        std::string test_str = R"(()";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "(", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // ComparisonNotEqual
    GROUP_TEST_F(LexerTest, OperatorTests, ComparisonNotEqual) {
        std::string test_str = R"(!=)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "!=", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // ComparisonEqual
    GROUP_TEST_F(LexerTest, OperatorTests, ComparisonEqual) {
        std::string test_str = R"(==)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "==", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // AndOperator
    GROUP_TEST_F(LexerTest, OperatorTests, AndOperator) {
        std::string test_str = R"(&&)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "&&", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // OrOperator
    GROUP_TEST_F(LexerTest, OperatorTests, OrOperator) {
        std::string test_str = R"(||)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "||", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // BitwiseAndOperator
    GROUP_TEST_F(LexerTest, OperatorTests, BitwiseAndOperator) {
        std::string test_str = R"(&)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "&", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // BitwiseOrOperator
    GROUP_TEST_F(LexerTest, OperatorTests, BitwiseOrOperator) {
        std::string test_str = R"(|)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "|", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // Semicolon
    GROUP_TEST_F(LexerTest, OperatorTests, Semicolon) {
        std::string test_str = R"(;)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, ";", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // MinusSign
    GROUP_TEST_F(LexerTest, OperatorTests, MinusSign) {
        std::string test_str = R"(-)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // PlusSign
    GROUP_TEST_F(LexerTest, OperatorTests, PlusSign) {
        std::string test_str = R"(+)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "+", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // FormFeedOperator
    GROUP_TEST_F(LexerTest, OperatorTests, FormFeedOperator) {
        std::string test_str = "\f";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\f", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // VerticalTabOperator
    GROUP_TEST_F(LexerTest, OperatorTests, VerticalTabOperator) {
        std::string test_str = "\u000B";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\u000B", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // TripleEquals
    GROUP_TEST_F(LexerTest, OperatorTests, TripleEquals) {
        std::string test_str = R"(===)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "===", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // TripleNotEquals
    GROUP_TEST_F(LexerTest, OperatorTests, TripleNotEquals) {
        std::string test_str = R"(!==)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "!==", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // NotAnd
    GROUP_TEST_F(LexerTest, OperatorTests, NotAnd) {
        std::string test_str = R"(!&&)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "!&&", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // NotOr
    GROUP_TEST_F(LexerTest, OperatorTests, NotOr) {
        std::string test_str = R"(!||)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "!||", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // IdentifierCharacterOperator
    GROUP_TEST_F(LexerTest, OperatorTests, IdentifierCharacterOperator) {
        std::string test_str = R"(a)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "a", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // DigitOperator
    GROUP_TEST_F(LexerTest, OperatorTests, DigitOperator) {
        std::string test_str = R"(1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "1", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // SpaceOperator
    GROUP_TEST_F(LexerTest, OperatorTests, SpaceOperator) {
        std::string test_str = " ";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, " ", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // TabOperator
    GROUP_TEST_F(LexerTest, OperatorTests, TabOperator) {
        std::string test_str = "\t";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\t", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // NewlineOperator
    GROUP_TEST_F(LexerTest, OperatorTests, NewlineOperator) {
        std::string test_str = "\n";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\n", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // CarriageReturnOperator
    GROUP_TEST_F(LexerTest, OperatorTests, CarriageReturnOperator) {
        std::string test_str = "\r";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\r", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }

    // BackspaceOperator
    GROUP_TEST_F(LexerTest, OperatorTests, BackspaceOperator) {
        std::string test_str = "\b";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\b", 0, 0, 0),
        };
        // success == false
        TEST_LEX(test_str, exp_tokens, false);
    }


    //
    // ===============
    // Example Tests
    // ===============
    //

    // Example1
    GROUP_TEST_F(LexerTest, ExampleTests, Example1) {
        std::string test_str = R"(LET x = 5;)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "LET", 0, 0, 0),
            Token(Token::Type::IDENTIFIER, "x", 4, 0, 4),
            Token(Token::Type::OPERATOR, "=", 6, 0, 6),
            Token(Token::Type::INTEGER, "5", 8, 0, 8),
            Token(Token::Type::OPERATOR, ";", 9, 0, 9),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // Example2
    GROUP_TEST_F(LexerTest, ExampleTests, Example2) {
        std::string test_str = R"(print("Hello, World!");)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "print", 0, 0, 0),
            Token(Token::Type::OPERATOR, "(", 5, 0, 5),
            Token(Token::Type::STRING, "\"Hello, World!\"", 6, 0, 6),
            Token(Token::Type::OPERATOR, ")", 21, 0, 21),
            Token(Token::Type::OPERATOR, ";", 22, 0, 22),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // StringExample
    GROUP_TEST_F(LexerTest, ExampleTests, StringExample) {
        std::string test_str = R"("Hello, World!" "This is string 2")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"Hello, World!\"", 0, 0, 0),
            Token(Token::Type::STRING, "\"This is string 2\"", 16, 0, 16),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // LeadingHyphenExample
    GROUP_TEST_F(LexerTest, ExampleTests, LeadingHyphenExample) {
        std::string test_str = R"(-five)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 0, 0),
            Token(Token::Type::IDENTIFIER, "five", 1, 0, 1),
        };
        // success == true (Note: in the Java test it recognized this as two tokens: OPERATOR "-" + IDENTIFIER "five")
        TEST_LEX(test_str, exp_tokens);
    }

    // LeadingDigits
    GROUP_TEST_F(LexerTest, ExampleTests, LeadingDigits) {
        std::string test_str = R"(1fish2fish3fishbluefish)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "1", 0, 0, 0),
            Token(Token::Type::IDENTIFIER, "fish2fish3fishbluefish", 1, 0, 1),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // SpacedIntegers
    GROUP_TEST_F(LexerTest, ExampleTests, SpacedIntegers) {
        std::string test_str = R"(05 394 98.81)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "05", 0, 0, 0),
            Token(Token::Type::INTEGER, "394", 3, 0, 3),
            Token(Token::Type::DECIMAL, "98.81", 7, 0, 7),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // TrailingNewline
    GROUP_TEST_F(LexerTest, ExampleTests, TrailingNewline) {
        std::string test_str = "token\n";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "token", 0, 0, 0),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeZeroDecimalWithMethodCall
    GROUP_TEST_F(LexerTest, ExampleTests, NegativeZeroDecimalWithMethodCall) {
        std::string test_str = R"(-0.0.toString())";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-0.0", 0, 0, 0),
            Token(Token::Type::OPERATOR, ".", 4, 0, 4),
            Token(Token::Type::IDENTIFIER, "toString", 5, 0, 5),
            Token(Token::Type::OPERATOR, "(", 13, 0, 13),
            Token(Token::Type::OPERATOR, ")", 14, 0, 14),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeOneTenthWithMethodCall
    GROUP_TEST_F(LexerTest, ExampleTests, NegativeOneTenthWithMethodCall) {
        std::string test_str = R"(-0.1.toString())";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-0.1", 0, 0, 0),
            Token(Token::Type::OPERATOR, ".", 4, 0, 4),
            Token(Token::Type::IDENTIFIER, "toString", 5, 0, 5),
            Token(Token::Type::OPERATOR, "(", 13, 0, 13),
            Token(Token::Type::OPERATOR, ")", 14, 0, 14),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeOneTenthTrailingZeroWithMethodCall
    GROUP_TEST_F(LexerTest, ExampleTests, NegativeOneTenthTrailingZeroWithMethodCall) {
        std::string test_str = R"(-0.10.toString())";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-0.10", 0, 0, 0),
            Token(Token::Type::OPERATOR, ".", 5, 0, 5),
            Token(Token::Type::IDENTIFIER, "toString", 6, 0, 6),
            Token(Token::Type::OPERATOR, "(", 14, 0, 14),
            Token(Token::Type::OPERATOR, ")", 15, 0, 15),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // NegativeOneDecimalWithMethodCall
    GROUP_TEST_F(LexerTest, ExampleTests, NegativeOneDecimalWithMethodCall) {
        std::string test_str = R"(-1.0.toString())";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "-1.0", 0, 0, 0),
            Token(Token::Type::OPERATOR, ".", 4, 0, 4),
            Token(Token::Type::IDENTIFIER, "toString", 5, 0, 5),
            Token(Token::Type::OPERATOR, "(", 13, 0, 13),
            Token(Token::Type::OPERATOR, ")", 14, 0, 14),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // MultipleDots
    GROUP_TEST_F(LexerTest, ExampleTests, MultipleDots) {
        std::string test_str = R"(1.2.3)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::DECIMAL, "1.2", 0, 0, 0),
            Token(Token::Type::DECIMAL, ".3", 3, 0, 3),
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

    // AllWhitespace
    GROUP_TEST_F(LexerTest, ExampleTests, AllWhitespace) {
        // Contains " \b\n\r\t"
        std::string test_str = " \b\n\r\t";
        std::vector<Token> exp_tokens {
            // no tokens recognized
        };
        // success == true
        TEST_LEX(test_str, exp_tokens);
    }

}