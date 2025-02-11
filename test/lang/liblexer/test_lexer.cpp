#include "fixtures/lexer_test.hpp"
#include "test_macros.hpp"
#include "token.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace BlsLang {

    // Identifier Tests
    GROUP_TEST_F(LexerTest, IdentifierTests, Alphabetic) {
        std::string test_str = R"(someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "someText", 0, 1, 1),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, Alphanumeric) {
        std::string test_str = R"(some1Text2)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "some1Text2", 0, 1, 1),  
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingDigit) {
        std::string test_str = R"(1someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "1someText", 0, 1, 1),  
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, LeadingUnderscore) {
        std::string test_str = R"(_someText)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "_someText", 0, 1, 1),  
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IdentifierTests, WhitespaceHandling) {
        std::string test_str = R"(   identifier   )";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "identifier", 3, 1, 4),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // Integer Tests
    GROUP_TEST_F(LexerTest, IntegerTests, IntegerLiteral) {
        std::string test_str = R"(120300)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "120300", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, NegativeInteger) {
        std::string test_str = R"(-450600)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::INTEGER, "450600", 1, 1, 2),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, NegativeZero) {
        std::string test_str = R"(-0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::INTEGER, "0", 1, 1, 2),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, HexLiteral) {
        std::string test_str = R"(0x123456789abcdef)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0x123456789abcdef", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, HexLiteralCapital) {
        std::string test_str = R"(0X123456789ABCDEF)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0X123456789ABCDEF", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, HexWithNonHexStartingDigit) {
        std::string test_str = R"(0xG)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0xG", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, HexWithNonHexLaterDigits) {
        std::string test_str = R"(0x12FfH)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0x12FfH", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, BinaryLiteral) {
        std::string test_str = R"(0b101101)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0b101101", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, BinaryLiteralCapital) {
        std::string test_str = R"(0B101101)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0B101101", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, BinaryWithNonBinaryStartingDigit) {
        std::string test_str = R"(0b2)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0b2", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, BinaryWithNonBinaryLaterDigits) {
        std::string test_str = R"(0b1019)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "0b1019", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, OctalLiteral) {
        std::string test_str = R"(001234)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "001234", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, OctalWithNonOctalStartingDigit) {
        std::string test_str = R"(09)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "09", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, IntegerTests, OctalWithNonOctalLaterDigits) {
        std::string test_str = R"(01239)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "01239", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    // Float Tests
    GROUP_TEST_F(LexerTest, FloatTests, FloatLiteralSingleIntAndFrac) {
        std::string test_str = R"(1.2)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "1.2", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, FloatLiteralZeroAndFrac) {
        std::string test_str = R"(0.2)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "0.2", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, FloatLiteralNoIntAndFrac) {
        std::string test_str = R"(.2)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, ".2", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, FloatLiteralMultiDigit) {
        std::string test_str = R"(10234.56789)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "10234.56789", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, NegativeDecimal) {
        std::string test_str = R"(-1.1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::FLOAT, "1.1", 1, 1, 2),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, NegativeDecimalFracOnly) {
        std::string test_str = R"(-.1)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::FLOAT, ".1", 1, 1, 2),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, ZeroDecimal) {
        std::string test_str = R"(0.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "0.0", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, ZeroDecimalFracOnly) {
        std::string test_str = R"(.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, ".0", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, NegativeZeroDecimal) {
        std::string test_str = R"(-0.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::FLOAT, "0.0", 1, 1, 2),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, NegativeZeroDecimalFracOnly) {
        std::string test_str = R"(-.0)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::FLOAT, ".0", 1, 1, 2),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, FloatTests, IntegerWithTrailingDecimalPoint) {
        std::string test_str = R"(1.)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "1.", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, FloatTests, ZeroWithTrailingDecimalPoint) {
        std::string test_str = R"(0.)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "0.", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, FloatTests, HexFloat) {
        std::string test_str = R"(0x123.456)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "0x123.456", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, FloatTests, BinaryFloat) {
        std::string test_str = R"(0b1010.101)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "0b1010.101", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, FloatTests, OctalFloat) {
        std::string test_str = R"(012.34)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "012.34", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    // String Tests
    GROUP_TEST_F(LexerTest, StringTests, Alphabetic) {
        std::string test_str = R"("This is a string")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"This is a string\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, AlphaNumeric) {
        std::string test_str = R"("This is a 5tring 123")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"This is a 5tring 123\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, SpecialCharacters) {
        std::string test_str = R"("`~!@#$%^&*()-_=+[{]}|;:,<.>/? ")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"`~!@#$%^&*()-_=+[{]}|;:,<.>/? \"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, EscapeSequences) {
        std::string test_str = R"(" \b\r\t\n\'\"\\ ")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\" \\b\\r\\t\\n\\'\\\"\\\\ \"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, EmptyString) {
        std::string test_str = R"("")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, EscapedNestedString) {
        std::string test_str = R"("\"\"")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\\"\\\"\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, ActiveBackspace) {
        std::string test_str = "\"\b\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\b\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, ActiveTab) {
        std::string test_str = "\"\t\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\t\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, ActiveFormFeed) {
        std::string test_str = "\"\f\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\f\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, ActiveVerticalTab) {
        std::string test_str = "\"\u000B\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\u000B\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, StringTests, NonEscapedBackslash) {
        std::string test_str = R"("\")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\\"", 0, 1, 1),
        };
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens, false), SyntaxError);
    }

    GROUP_TEST_F(LexerTest, StringTests, LiteralEmptyString) {
        std::string test_str = "";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, StringTests, InvalidEscapeSequences) {
        std::string test_str = "\"\\q\\w\\e\\1\\2\\`\\&\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\\q\\w\\e\\1\\2\\`\\&\"", 0, 1, 1),
        };
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), SyntaxError);
    }

    GROUP_TEST_F(LexerTest, StringTests, Unterminated) {
        std::string test_str = R"("unterminated)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"unterminated", 0, 1, 1),
        };
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), SyntaxError);
    }

    GROUP_TEST_F(LexerTest, StringTests, UnescapedNestedString) {
        std::string test_str = R"("" "")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\"\"\"", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, StringTests, StringLiteralBrokenByNewline) {
        std::string test_str = "\"\n\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\n\"", 0, 1, 1),
        };
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), SyntaxError);
    }

    GROUP_TEST_F(LexerTest, StringTests, StringLiteralBrokenByCarriageReturn) {
        std::string test_str = "\"\r\"";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"\r\"", 0, 1, 1),
        };
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), SyntaxError);
    }

    // Comment Tests
    GROUP_TEST_F(LexerTest, CommentTests, SingleLineComment) {
        std::string test_str = R"(// This is a comment)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "// This is a comment", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, CommentTests, MultiLineComment) {
        std::string test_str = "/* Multi-line\ncomment */";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "/* Multi-line\ncomment */", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, CommentTests, UnclosedComment) {
        std::string test_str = R"(/* Multiline comment unclosed)";
        std::vector<Token> exp_tokens {};
        EXPECT_THROW(TEST_LEX(test_str, exp_tokens), SyntaxError);
    }

    GROUP_TEST_F(LexerTest, CommentTests, NestedComments) {
        std::string test_str = R"(/* Outer /* Inner */ Still outer */)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "/* Outer /* Inner */", 0, 1, 1),
            Token(Token::Type::IDENTIFIER, "Still", 21, 1, 22),
            Token(Token::Type::IDENTIFIER, "outer", 27, 1, 28),
            Token(Token::Type::OPERATOR, "*", 33, 1, 34),
            Token(Token::Type::OPERATOR, "/", 34, 1, 35),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, CommentTests, NestedCommentsUnclosed) {
        std::string test_str = R"(/* Outer /* Inner Still outer */)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::COMMENT, "/* Outer /* Inner Still outer */", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // Operator Tests
    GROUP_TEST_F(LexerTest, OperatorTests, SimpleOperator) {
        std::string test_str = R"(+)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "+", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, CompoundOperatorEquals) {
        std::string test_str = R"(<=)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "<=", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, IncrementOperator) {
        std::string test_str = R"(++)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "++", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, DecrementOperator) {
        std::string test_str = R"(--)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "--", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, AndOperator) {
        std::string test_str = R"(&&)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "&&", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, OrOperator) {
        std::string test_str = R"(||)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "||", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, MixedCompounds) {
        std::string test_str = R"(<=>=!===+=-=*=/=^=%=)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "<=", 0, 1, 1),
            Token(Token::Type::OPERATOR, ">=", 2, 1, 3),
            Token(Token::Type::OPERATOR, "!=", 4, 1, 5),
            Token(Token::Type::OPERATOR, "==", 6, 1, 7),
            Token(Token::Type::OPERATOR, "+=", 8, 1, 9),
            Token(Token::Type::OPERATOR, "-=", 10, 1, 11),
            Token(Token::Type::OPERATOR, "*=", 12, 1, 13),
            Token(Token::Type::OPERATOR, "/=", 14, 1, 15),
            Token(Token::Type::OPERATOR, "^=", 16, 1, 17),
            Token(Token::Type::OPERATOR, "%=", 18, 1, 19),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, FormFeedOperator) {
        std::string test_str = "\f";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\f", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, VerticalTabOperator) {
        std::string test_str = "\u000B";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\u000B", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, NewlineOperator) {
        std::string test_str = "\n";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\n", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, CarriageReturnOperator) {
        std::string test_str = "\r";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\r", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    GROUP_TEST_F(LexerTest, OperatorTests, BackspaceOperator) {
        std::string test_str = "\b";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "\b", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens, false);
    }

    // Example Tests
    GROUP_TEST_F(LexerTest, ExampleTests, EmptyInput) {
        std::string test_str = R"()";
        std::vector<Token> exp_tokens {};
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, AllWhitespace) {
        std::string test_str = " \u000B\u0008\n\r\f\t";
        std::vector<Token> exp_tokens {};
        TEST_LEX(test_str, exp_tokens);
    }


    GROUP_TEST_F(LexerTest, ExampleTests, MixedWhitespaceAndTokens) {
        std::string test_str = "  id1\t+ \n   id2  ";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "id1", 2, 1, 3),
            Token(Token::Type::OPERATOR, "+", 6, 1, 7),
            Token(Token::Type::IDENTIFIER, "id2", 12, 2, 4),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, CompoundOperatorsAndIdentifiers) {
        std::string test_str = R"(id1++; id2--;)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "id1", 0, 1, 1),
            Token(Token::Type::OPERATOR, "++", 3, 1, 4),
            Token(Token::Type::OPERATOR, ";", 5, 1, 6),
            Token(Token::Type::IDENTIFIER, "id2", 7, 1, 8),
            Token(Token::Type::OPERATOR, "--", 10, 1, 11),
            Token(Token::Type::OPERATOR, ";", 12, 1, 13),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, Expression) {
        std::string test_str = R"(int x = 5;)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "int", 0, 1, 1),
            Token(Token::Type::IDENTIFIER, "x", 4, 1, 5),
            Token(Token::Type::OPERATOR, "=", 6, 1, 7),
            Token(Token::Type::INTEGER, "5", 8, 1, 9),
            Token(Token::Type::OPERATOR, ";", 9, 1, 10),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, FunctionCall) {
        std::string test_str = R"(print("Hello, World!");)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "print", 0, 1, 1),
            Token(Token::Type::OPERATOR, "(", 5, 1, 6),
            Token(Token::Type::STRING, "\"Hello, World!\"", 6, 1, 7),
            Token(Token::Type::OPERATOR, ")", 21, 1, 22),
            Token(Token::Type::OPERATOR, ";", 22, 1, 23),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, UnspacedStrings) {
        std::string test_str = R"("Hello, World!""This is string 2")";
        std::vector<Token> exp_tokens {
            Token(Token::Type::STRING, "\"Hello, World!\"", 0, 1, 1),
            Token(Token::Type::STRING, "\"This is string 2\"", 15, 1, 16),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    // SpacedIntegers
    GROUP_TEST_F(LexerTest, ExampleTests, SpacedIntegers) {
        std::string test_str = R"(15 394 98.81 0 0x9F 0b110 017)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::INTEGER, "15", 0, 1, 1),
            Token(Token::Type::INTEGER, "394", 3, 1, 4),
            Token(Token::Type::FLOAT, "98.81", 7, 1, 8),
            Token(Token::Type::INTEGER, "0", 13, 1, 14),
            Token(Token::Type::INTEGER, "0x9F", 15, 1, 16),
            Token(Token::Type::INTEGER, "0b110", 20, 1, 21),
            Token(Token::Type::INTEGER, "017", 26, 1, 27),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, TrailingNewline) {
        std::string test_str = "token\n";
        std::vector<Token> exp_tokens {
            Token(Token::Type::IDENTIFIER, "token", 0, 1, 1),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, FloatWithMethodCall) {
        std::string test_str = R"(1.0.toString())";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "1.0", 0, 1, 1),
            Token(Token::Type::OPERATOR, ".", 3, 1, 4),
            Token(Token::Type::IDENTIFIER, "toString", 4, 1, 5),
            Token(Token::Type::OPERATOR, "(", 12, 1, 13),
            Token(Token::Type::OPERATOR, ")", 13, 1, 14),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, NegativeFloatWithMethodCall) {
        std::string test_str = R"(-1.0.toString())";
        std::vector<Token> exp_tokens {
            Token(Token::Type::OPERATOR, "-", 0, 1, 1),
            Token(Token::Type::FLOAT, "1.0", 1, 1, 2),
            Token(Token::Type::OPERATOR, ".", 4, 1, 5),
            Token(Token::Type::IDENTIFIER, "toString", 5, 1, 6),
            Token(Token::Type::OPERATOR, "(", 13, 1, 14),
            Token(Token::Type::OPERATOR, ")", 14, 1, 15),
        };
        TEST_LEX(test_str, exp_tokens);
    }

    GROUP_TEST_F(LexerTest, ExampleTests, MultipleDots) {
        std::string test_str = R"(1.2.3)";
        std::vector<Token> exp_tokens {
            Token(Token::Type::FLOAT, "1.2", 0, 1, 1),
            Token(Token::Type::FLOAT, ".3", 3, 1, 4),
        };
        TEST_LEX(test_str, exp_tokens);
    }

}