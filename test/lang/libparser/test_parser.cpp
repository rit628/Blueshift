#include "ast.hpp"
#include "fixtures/parser_test.hpp"
#include "parser.hpp"
#include "test_macros.hpp"
#include "error_types.hpp"
#include "include/reserved_tokens.hpp"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(ParserTest, SpecifierTests, SimpleType) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0, 1, 1)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            PRIMITIVE_INT,
            {}
        ));
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SpecifierTests, ContainerType) {
        // Example: list<int>
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "list", 0, 1, 1),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 2, 1, 3),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE, 3, 1, 4)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            "list",
            {
                new AstNode::Specifier::Type(
                    PRIMITIVE_INT,
                    {}
                )
            }
        ));
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SpecifierTests, NestedContainerType) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "list", 0, 1, 1),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "list", 2, 1, 3),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN, 3, 1, 4),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_FLOAT, 4, 1, 5),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE, 5, 1, 6),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE, 6, 1, 7)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            "list",
            {
                new AstNode::Specifier::Type(
                    "list",
                    {
                        new AstNode::Specifier::Type(PRIMITIVE_FLOAT, {})
                    }
                )
            }
        ));
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SpecifierTests, MultipleTypeArguments) {
        // map<int, string>
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "map", 0, 1, 1),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 2, 1, 3),
            Token(Token::Type::OPERATOR, COMMA, 3, 1, 4),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_STRING, 4, 1, 5),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE, 5, 1, 6)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            "map",
            {
                new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
                new AstNode::Specifier::Type(PRIMITIVE_STRING, {})
            }
        ));
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralInteger) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "123", 0, 1, 1)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            (size_t) 123
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralFloat) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::FLOAT, "6.28", 0, 1, 1)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            6.28
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralString) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::STRING, R"("hello \n \\ \"wrld\" ")", 0, 1, 1)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            std::string("hello \n \\ \"wrld\" ")
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralBoolean) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE, 0, 1, 1)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            true
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, ListExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, BRACKET_OPEN, 0, 1, 1),
            Token(Token::Type::INTEGER, "1", 1, 1, 2),
            Token(Token::Type::OPERATOR, COMMA, 2, 1, 3),
            Token(Token::Type::INTEGER, "2", 3, 1, 4),
            Token(Token::Type::OPERATOR, COMMA, 4, 1, 5),
            Token(Token::Type::INTEGER, "3", 5, 1, 6),
            Token(Token::Type::OPERATOR, BRACKET_CLOSE, 6, 1, 7)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::List(
            {
                new AstNode::Expression::Literal((size_t)1),
                new AstNode::Expression::Literal((size_t)2),
                new AstNode::Expression::Literal((size_t)3)
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryAddition) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "2", 0, 1, 1),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION, 1, 1, 2),
            Token(Token::Type::INTEGER, "3", 2, 1, 3)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ARITHMETIC_ADDITION,
            new AstNode::Expression::Literal(
                (size_t) 2
            ),
            new AstNode::Expression::Literal(
                (size_t) 3
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryAssignment) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a", 0, 1, 1),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 1, 1, 2),
            Token(Token::Type::INTEGER, "10", 2, 1, 3),
            Token(Token::Type::OPERATOR, SEMICOLON, 3, 1, 4)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ASSIGNMENT,
            new AstNode::Expression::Access("a"),
            new AstNode::Expression::Literal((size_t) 10)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryCompoundAssignment) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a", 0, 1, 1),
            Token(Token::Type::OPERATOR, ASSIGNMENT_ADDITION, 1, 1, 2),
            Token(Token::Type::INTEGER, "10", 2, 1, 3),
            Token(Token::Type::OPERATOR, SEMICOLON, 3, 1, 4)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ASSIGNMENT_ADDITION,
            new AstNode::Expression::Access("a"),
            new AstNode::Expression::Literal((size_t) 10)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, ComplexBinaryExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "1", 0, 1, 1),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION, 1, 1, 2),
            Token(Token::Type::INTEGER, "2", 2, 1, 3),
            Token(Token::Type::OPERATOR, ARITHMETIC_MULTIPLICATION, 3, 1, 4),
            Token(Token::Type::INTEGER, "3", 4, 1, 5)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ARITHMETIC_ADDITION,
            new AstNode::Expression::Literal((size_t)1),
            new AstNode::Expression::Binary(
                ARITHMETIC_MULTIPLICATION,
                new AstNode::Expression::Literal((size_t)2),
                new AstNode::Expression::Literal((size_t)3)
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, GroupedBinaryExpression) {
        // (a + b) * c
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 0, 1, 1),
            Token(Token::Type::IDENTIFIER, "a", 1, 1, 2),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION, 2, 1, 3),
            Token(Token::Type::IDENTIFIER, "b", 3, 1, 4),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 4, 1, 5),
            Token(Token::Type::OPERATOR, ARITHMETIC_MULTIPLICATION, 5, 1, 6),
            Token(Token::Type::IDENTIFIER, "c", 6, 1, 7)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ARITHMETIC_MULTIPLICATION,
            new AstNode::Expression::Group(
                new AstNode::Expression::Binary(
                    ARITHMETIC_ADDITION,
                    new AstNode::Expression::Access("a"),
                    new AstNode::Expression::Access("b")
                )
            ),
            new AstNode::Expression::Access("c")
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, UnaryNegative) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, UNARY_NEGATIVE, 0, 1, 1),
            Token(Token::Type::INTEGER, "5", 1, 1, 2)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            UNARY_NEGATIVE,
            new AstNode::Expression::Literal(
                (size_t) 5
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, PostfixUnaryExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a", 0, 1, 1),
            Token(Token::Type::OPERATOR, UNARY_INCREMENT, 1, 1, 2)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            UNARY_INCREMENT,
            new AstNode::Expression::Access("a"),
            AstNode::Expression::Unary::OPERATOR_POSITION::POSTFIX
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, GroupedExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 0, 1, 1),
            Token(Token::Type::INTEGER, "7", 1, 1, 2),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 2, 1, 3)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Group(
            new AstNode::Expression::Literal(
                (size_t) 7
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, FunctionCallExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "foo", 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            Token(Token::Type::INTEGER, "42", 2, 1, 3),
            Token(Token::Type::OPERATOR, COMMA, 3, 1, 4),
            Token(Token::Type::FLOAT, "3.14", 4, 1, 5),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 5, 1, 6)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Function(
            "foo",
            {
                new AstNode::Expression::Literal((size_t) 42),
                new AstNode::Expression::Literal(3.14)
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, NestedFunctionCallExpression) {
        // foo(bar(1), baz(2))
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "foo", 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
                // First argument: bar(1)
                Token(Token::Type::IDENTIFIER, "bar", 2, 1, 3),
                Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 3, 1, 4),
                Token(Token::Type::INTEGER, "1", 4, 1, 5),
                Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 5, 1, 6),
                Token(Token::Type::OPERATOR, COMMA, 6, 1, 7),
                // Second argument: baz(2)
                Token(Token::Type::IDENTIFIER, "baz", 7, 1, 8),
                Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 8, 1, 9),
                Token(Token::Type::INTEGER, "2", 9, 1, 10),
                Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 10, 1, 11),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 11, 1, 12)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Function(
            "foo",
            {
                new AstNode::Expression::Function("bar", { new AstNode::Expression::Literal((size_t)1) }),
                new AstNode::Expression::Function("baz", { new AstNode::Expression::Literal((size_t)2) })
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, MethodCallExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "obj", 0, 1, 1),
            Token(Token::Type::OPERATOR, MEMBER_ACCESS, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "method", 2, 1, 3),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 3, 1, 4),
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE, 4, 1, 5),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 5, 1, 6)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Method(
            "obj",
            "method",
            {
                new AstNode::Expression::Literal(true)
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, MethodCallMultipleArguments) {
        // obj.method(10, 20)
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "obj", 0, 1, 1),
            Token(Token::Type::OPERATOR, MEMBER_ACCESS, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "method", 2, 1, 3),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 3, 1, 4),
            Token(Token::Type::INTEGER, "10", 4, 1, 5),
            Token(Token::Type::OPERATOR, COMMA, 5, 1, 6),
            Token(Token::Type::INTEGER, "20", 6, 1, 7),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 7, 1, 8)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Method(
            "obj", "method",
            {
                new AstNode::Expression::Literal((size_t)10),
                new AstNode::Expression::Literal((size_t)20)
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, MemberAccessExpression) {
        // obj.field
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "obj", 0, 1, 1),
            Token(Token::Type::OPERATOR, MEMBER_ACCESS, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "field", 2, 1, 3)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "obj", "field"
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, AccessSubscript) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "arr", 0, 1, 1),
            Token(Token::Type::OPERATOR, BRACKET_OPEN, 1, 1, 2),
            Token(Token::Type::INTEGER, "0", 2, 1, 3),
            Token(Token::Type::OPERATOR, BRACKET_CLOSE, 3, 1, 4)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "arr",
            new AstNode::Expression::Literal((size_t) 0)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, UnmatchedParenthesis) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 0, 1, 1),
            Token(Token::Type::INTEGER, "42", 1, 1, 2)
        };
        EXPECT_THROW(TEST_PARSE_EXPRESSION(sampleTokens, nullptr), SyntaxError);
    }

    GROUP_TEST_F(ParserTest, StatementTests, ExpressionStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "foo", 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            Token(Token::Type::INTEGER, "42", 2, 1, 3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3, 1, 4),
            Token(Token::Type::OPERATOR, SEMICOLON, 4, 1, 5)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Expression(
            new AstNode::Expression::Function("foo", { new AstNode::Expression::Literal((size_t)42) })
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }
    
    GROUP_TEST_F(ParserTest, StatementTests, DeclarationStatement) {
        // Declaration statement: int x = 10;
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0, 1, 1),
            Token(Token::Type::IDENTIFIER, "x", 1, 1, 2),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 2, 1, 3),
            Token(Token::Type::INTEGER, "10", 3, 1, 4),
            Token(Token::Type::OPERATOR, SEMICOLON, 4, 1, 5)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
            new AstNode::Expression::Literal((size_t)10)
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ReturnStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 0, 1, 1),
            Token(Token::Type::INTEGER, "42", 1, 1, 2),
            Token(Token::Type::OPERATOR, SEMICOLON, 2, 1, 3)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Return(
            new AstNode::Expression::Literal((size_t) 42)
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ReturnVoidStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 0, 1, 1),
            Token(Token::Type::OPERATOR, SEMICOLON, 1, 1, 2)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Return(
            std::nullopt
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, IfElseStatement) {
        // if ( a ) { b = 1; } else { b = 2; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_IF, 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "a", 2, 1, 3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3, 1, 4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4, 1, 5),
            Token(Token::Type::IDENTIFIER, "b", 5, 1, 6),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 6, 1, 7),
            Token(Token::Type::INTEGER, "1", 7, 1, 8),
            Token(Token::Type::OPERATOR, SEMICOLON, 8, 1, 9),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 9, 1, 10),
            Token(Token::Type::IDENTIFIER, RESERVED_ELSE, 10, 1, 11),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 11, 1, 12),
            Token(Token::Type::IDENTIFIER, "b", 12, 1, 13),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 13, 1, 14),
            Token(Token::Type::INTEGER, "2", 14, 1, 15),
            Token(Token::Type::OPERATOR, SEMICOLON, 15, 1, 16),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 16, 1, 17)
        };

        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Access("a"),
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("b"),
                    new AstNode::Expression::Literal((size_t) 1)
                ))
            },
            {},
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("b"),
                    new AstNode::Expression::Literal((size_t) 2)
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, IfElseIfStatement) {
        // if (a) { x = 1; } else if (b) { x = 2; } else { x = 3; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_IF, 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "a", 2, 1, 3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3, 1, 4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4, 1, 5),
            Token(Token::Type::IDENTIFIER, "x", 5, 1, 6),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 6, 1, 7),
            Token(Token::Type::INTEGER, "1", 7, 1, 8),
            Token(Token::Type::OPERATOR, SEMICOLON, 8, 1, 9),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 9, 1, 10),
            Token(Token::Type::IDENTIFIER, RESERVED_ELSE, 10, 1, 11),
            Token(Token::Type::IDENTIFIER, RESERVED_IF, 11, 1, 12),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 12, 1, 13),
            Token(Token::Type::IDENTIFIER, "b", 13, 1, 14),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 14, 1, 15),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 15, 1, 16),
            Token(Token::Type::IDENTIFIER, "x", 16, 1, 17),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 17, 1, 18),
            Token(Token::Type::INTEGER, "2", 18, 1, 19),
            Token(Token::Type::OPERATOR, SEMICOLON, 19, 1, 20),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 20, 1, 21),
            Token(Token::Type::IDENTIFIER, RESERVED_ELSE, 21, 1, 22),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 22, 1, 23),
            Token(Token::Type::IDENTIFIER, "x", 23, 1, 24),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 24, 1, 25),
            Token(Token::Type::INTEGER, "3", 25, 1, 26),
            Token(Token::Type::OPERATOR, SEMICOLON, 26, 1, 27),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 27, 1, 28)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Access("a"),
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("x"),
                    new AstNode::Expression::Literal((size_t)1)
                ))
            },
            {
                new AstNode::Statement::If(
                    new AstNode::Expression::Access("b"),
                    {
                        new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                            ASSIGNMENT,
                            new AstNode::Expression::Access("x"),
                            new AstNode::Expression::Literal((size_t)2)
                        ))
                    },
                    {},
                    {}
                )
            },
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("x"),
                    new AstNode::Expression::Literal((size_t)3)
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, WhileStatement) {
        // while ( a ) { a = a - 1; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_WHILE, 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, "a", 2, 1, 3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3, 1, 4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4, 1, 5),
            Token(Token::Type::IDENTIFIER, "a", 5, 1, 6),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 6, 1, 7),
            Token(Token::Type::IDENTIFIER, "a", 7, 1, 8),
            Token(Token::Type::OPERATOR, ARITHMETIC_SUBTRACTION, 8, 1, 9),
            Token(Token::Type::INTEGER, "1", 9, 1, 10),
            Token(Token::Type::OPERATOR, SEMICOLON, 10, 1, 11),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 11, 1, 12)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Access("a"),
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("a"),
                    new AstNode::Expression::Binary(
                        ARITHMETIC_SUBTRACTION,
                        new AstNode::Expression::Access("a"),
                        new AstNode::Expression::Literal((size_t) 1)
                    )
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ForStatement) {
        // for ( int i = 0; i < 10; i++ ) { sum = sum + i; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_FOR, 0,1,1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1,1,2),
            // Init: int i = 0;
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 2,1,3),
            Token(Token::Type::IDENTIFIER, "i", 3,1,4),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 4,1,5),
            Token(Token::Type::INTEGER, "0", 5,1,6),
            Token(Token::Type::OPERATOR, SEMICOLON, 6,1,7),
            // Condition: i < 10;
            Token(Token::Type::IDENTIFIER, "i", 7,1,8),
            Token(Token::Type::OPERATOR, COMPARISON_LT, 8,1,9),
            Token(Token::Type::INTEGER, "10", 9,1,10),
            Token(Token::Type::OPERATOR, SEMICOLON, 10,1,11),
            // Increment: i++
            Token(Token::Type::IDENTIFIER, "i", 11,1,12),
            Token(Token::Type::OPERATOR, UNARY_INCREMENT, 12,1,13),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 13,1,14),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 14,1,15),
            // Block: sum = sum + i;
            Token(Token::Type::IDENTIFIER, "sum", 15,1,16),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 16,1,17),
            Token(Token::Type::IDENTIFIER, "sum", 17,1,18),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION, 18,1,19),
            Token(Token::Type::IDENTIFIER, "i", 19,1,20),
            Token(Token::Type::OPERATOR, SEMICOLON, 20,1,21),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 21,1,22)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            new AstNode::Statement::Declaration(
                "i",
                new AstNode::Specifier::Type(
                    PRIMITIVE_INT,
                    {}
                ),
                new AstNode::Expression::Literal((size_t) 0)
            ),
            new AstNode::Statement::Expression(
                new AstNode::Expression::Binary(
                    COMPARISON_LT,
                    new AstNode::Expression::Access("i"),
                    new AstNode::Expression::Literal((size_t) 10)
                )
            ),
            new AstNode::Expression::Unary(
                UNARY_INCREMENT,
                new AstNode::Expression::Access("i"),
                AstNode::Expression::Unary::OPERATOR_POSITION::POSTFIX
            ),
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("sum"),
                    new AstNode::Expression::Binary(
                        ARITHMETIC_ADDITION,
                        new AstNode::Expression::Access("sum"),
                        new AstNode::Expression::Access("i")
                    )
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, EmptyForStatement) {
        // For loop with empty init, condition, and increment: for (;;){ foo(); }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_FOR, 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            // Empty init statement
            Token(Token::Type::OPERATOR, SEMICOLON, 2, 1, 3),
            // Empty condition
            Token(Token::Type::OPERATOR, SEMICOLON, 3, 1, 4),
            // Empty increment expression, then closing parenthesis
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 4, 1, 5),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 5, 1, 6),
                // Block: foo();
                Token(Token::Type::IDENTIFIER, "foo", 6, 1, 7),
                Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 7, 1, 8),
                Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 8, 1, 9),
                Token(Token::Type::OPERATOR, SEMICOLON, 9, 1, 10),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 10, 1, 11)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Function("foo", {})
                )
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, MissingSemicolonStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "x", 0, 1, 1),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 1, 1, 2),
            Token(Token::Type::INTEGER, "5", 2, 1, 3)
        };
        EXPECT_THROW(TEST_PARSE_STATEMENT(sampleTokens, nullptr), SyntaxError);
    }


    GROUP_TEST_F(ParserTest, FunctionTests, Procedure) {
        // int main() { return 0; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0,1,1),
            Token(Token::Type::IDENTIFIER, "main", 1,1,2),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 2,1,3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3,1,4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4,1,5),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 5,1,6),
            Token(Token::Type::INTEGER, "0", 6,1,7),
            Token(Token::Type::OPERATOR, SEMICOLON, 7,1,8),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 8,1,9)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "main",
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Literal((size_t) 0)
                )
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, ProcedureWithParameters) {
        // Procedure with parameters: int add(int a, int b) { return a + b; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0,1,1),
            Token(Token::Type::IDENTIFIER, "add", 1,1,2),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 2,1,3),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 3,1,4),
            Token(Token::Type::IDENTIFIER, "a", 4,1,5),
            Token(Token::Type::OPERATOR, COMMA, 5,1,6),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 6,1,7),
            Token(Token::Type::IDENTIFIER, "b", 7,1,8),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 8,1,9),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 9,1,10),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 10,1,11),
            Token(Token::Type::IDENTIFIER, "a", 11,1,12),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION, 12,1,13),
            Token(Token::Type::IDENTIFIER, "b", 13,1,14),
            Token(Token::Type::OPERATOR, SEMICOLON, 14,1,15),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 15,1,16)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "add",
            new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
            {
                new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
                new AstNode::Specifier::Type(PRIMITIVE_INT, {})
            },
            { "a", "b" },
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Binary(
                        ARITHMETIC_ADDITION,
                        new AstNode::Expression::Access("a"),
                        new AstNode::Expression::Access("b")
                    )
                )
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, Oblock) {
        // oblock foo() { a = 1; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_OBLOCK, 0,1,1),
            Token(Token::Type::IDENTIFIER, "foo", 1,1,2),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 2,1,3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3,1,4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4,1,5),
            Token(Token::Type::IDENTIFIER, "a", 5,1,6),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 6,1,7),
            Token(Token::Type::INTEGER, "1", 7,1,8),
            Token(Token::Type::OPERATOR, SEMICOLON, 8,1,9),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 9,1,10)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Oblock(
            "foo",
            {},
            {},
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("a"),
                    new AstNode::Expression::Literal((size_t) 1)
                ))
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, OblockMultipleStatements) {
        // Oblock with multiple statements: oblock init() { x = 0; y = 0; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_OBLOCK, 0,1,1),
            Token(Token::Type::IDENTIFIER, "init", 1,1,2),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 2,1,3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3,1,4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4,1,5),
            Token(Token::Type::IDENTIFIER, "x", 5,1,6),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 6,1,7),
            Token(Token::Type::INTEGER, "0", 7,1,8),
            Token(Token::Type::OPERATOR, SEMICOLON, 8,1,9),
            Token(Token::Type::IDENTIFIER, "y", 9,1,10),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 10,1,11),
            Token(Token::Type::INTEGER, "0", 11,1,12),
            Token(Token::Type::OPERATOR, SEMICOLON, 12,1,13),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 13,1,14)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Oblock(
            "init",
            {},
            {},
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("x"),
                    new AstNode::Expression::Literal((size_t)0)
                )),
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("y"),
                    new AstNode::Expression::Literal((size_t)0)
                ))
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, MissingFunctionName) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0, 1, 1),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 1, 1, 2),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 2, 1, 3),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 3, 1, 4),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 4, 1, 5)
        };
        EXPECT_THROW(TEST_PARSE_FUNCTION(sampleTokens, nullptr), SyntaxError);
    }

    GROUP_TEST_F(ParserTest, SourceTests, FullSource) {
        // Full source: one procedure, one oblock, and one setup.
        // Procedure: int main() { return 0; }
        // Oblock: oblock helper() { a = 1; }
        // Setup: setup() { int x = 5; }
        std::vector<Token> sampleTokens {
            // Procedure:
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0,1,1),
            Token(Token::Type::IDENTIFIER, "main", 1,1,2),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 2,1,3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3,1,4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4,1,5),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 5,1,6),
            Token(Token::Type::INTEGER, "0", 6,1,7),
            Token(Token::Type::OPERATOR, SEMICOLON, 7,1,8),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 8,1,9),
            // Oblock:
            Token(Token::Type::IDENTIFIER, RESERVED_OBLOCK, 9,1,10),
            Token(Token::Type::IDENTIFIER, "helper", 10,1,11),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 11,1,12),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 12,1,13),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 13,1,14),
            Token(Token::Type::IDENTIFIER, "a", 14,1,15),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 15,1,16),
            Token(Token::Type::INTEGER, "1", 16,1,17),
            Token(Token::Type::OPERATOR, SEMICOLON, 17,1,18),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 18,1,19),
            // Setup:
            Token(Token::Type::IDENTIFIER, RESERVED_SETUP, 19,1,20),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 20,1,21),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 21,1,22),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 22,1,23),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 23,1,24),
            Token(Token::Type::IDENTIFIER, "x", 24,1,25),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 25,1,26),
            Token(Token::Type::INTEGER, "5", 26,1,27),
            Token(Token::Type::OPERATOR, SEMICOLON, 27,1,28),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 28,1,29)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "main",
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    {},
                    {},
                    {
                        new AstNode::Statement::Return(
                            new AstNode::Expression::Literal((size_t) 0)
                        )
                    }
                )
            },
            {
                new AstNode::Function::Oblock(
                    "helper",
                    {},
                    {},
                    {
                        new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                            ASSIGNMENT,
                            new AstNode::Expression::Access("a"),
                            new AstNode::Expression::Literal((size_t) 1)
                        ))
                    }
                )
            },
            new AstNode::Setup(
                {
                    new AstNode::Statement::Declaration(
                        "x",
                        new AstNode::Specifier::Type(
                            PRIMITIVE_INT,
                            {}
                        ),
                        new AstNode::Expression::Literal((size_t) 5)
                    )
                }
            )
        ));
        TEST_PARSE_SOURCE(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SourceTests, MultipleProceduresAndOblocks) {
        std::vector<Token> sampleTokens {
            // Procedure 1: int main() { return 0; }
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0,1,1),
            Token(Token::Type::IDENTIFIER, "main", 1,1,2),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 2,1,3),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 3,1,4),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 4,1,5),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 5,1,6),
            Token(Token::Type::INTEGER, "0", 6,1,7),
            Token(Token::Type::OPERATOR, SEMICOLON, 7,1,8),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 8,1,9),
            // Procedure 2: int helper() { return 1; }
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 9,1,10),
            Token(Token::Type::IDENTIFIER, "helper", 10,1,11),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 11,1,12),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 12,1,13),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 13,1,14),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 14,1,15),
            Token(Token::Type::INTEGER, "1", 15,1,16),
            Token(Token::Type::OPERATOR, SEMICOLON, 16,1,17),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 17,1,18),
            // Oblock: oblock config() { flag = true; }
            Token(Token::Type::IDENTIFIER, RESERVED_OBLOCK, 18,1,19),
            Token(Token::Type::IDENTIFIER, "config", 19,1,20),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 20,1,21),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 21,1,22),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 22,1,23),
            Token(Token::Type::IDENTIFIER, "flag", 23,1,24),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 24,1,25),
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE, 25,1,26),
            Token(Token::Type::OPERATOR, SEMICOLON, 26,1,27),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 27,1,28),
            // Setup: setup() { }
            Token(Token::Type::IDENTIFIER, RESERVED_SETUP, 28,1,29),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 29,1,30),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 30,1,31),
            Token(Token::Type::OPERATOR, BRACE_OPEN, 31,1,32),
            Token(Token::Type::OPERATOR, BRACE_CLOSE, 32,1,33)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "main",
                    new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
                    {},
                    {},
                    {
                        new AstNode::Statement::Return(
                            new AstNode::Expression::Literal((size_t)0)
                        )
                    }
                ),
                new AstNode::Function::Procedure(
                    "helper",
                    new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
                    {},
                    {},
                    {
                        new AstNode::Statement::Return(
                            new AstNode::Expression::Literal((size_t)1)
                        )
                    }
                )
            },
            {
                new AstNode::Function::Oblock(
                    "config",
                    {},
                    {},
                    {
                        new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                            ASSIGNMENT,
                            new AstNode::Expression::Access("flag"),
                            new AstNode::Expression::Literal(true)
                        ))
                    }
                )
            },
            new AstNode::Setup({})  // Empty setup block
        ));
        TEST_PARSE_SOURCE(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SourceTests, EmptySource) {
        std::vector<Token> sampleTokens {};
        EXPECT_THROW(TEST_PARSE_SOURCE(sampleTokens, nullptr), SyntaxError);
    }

    GROUP_TEST_F(ParserTest, SourceTests, InvalidTopLevelElement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, "?", 0, 1, 1)
        };
        EXPECT_THROW(TEST_PARSE_SOURCE(sampleTokens, nullptr), SyntaxError);
    }

}