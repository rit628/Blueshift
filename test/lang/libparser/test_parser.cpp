#include "ast.hpp"
#include "fixtures/parser_test.hpp"
#include "parser.hpp"
#include "test_macros.hpp"
#include "error_types.hpp"
#include "include/reserved_tokens.hpp"
#include "token.hpp"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(ParserTest, SpecifierTests, SimpleType) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT)
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
            Token(Token::Type::IDENTIFIER, CONTAINER_LIST),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            CONTAINER_LIST,
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
            Token(Token::Type::IDENTIFIER, CONTAINER_LIST),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN),
            Token(Token::Type::IDENTIFIER, CONTAINER_LIST),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_FLOAT),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            CONTAINER_LIST,
            {
                new AstNode::Specifier::Type(
                    CONTAINER_LIST,
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
            Token(Token::Type::IDENTIFIER, CONTAINER_MAP),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_STRING),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Specifier::Type(
            CONTAINER_MAP,
            {
                new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
                new AstNode::Specifier::Type(PRIMITIVE_STRING, {})
            }
        ));
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralInteger) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "123")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            static_cast<int64_t>(123)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralBinaryInteger) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "0b101")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            static_cast<int64_t>(5)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralOctalInteger) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "071")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            static_cast<int64_t>(57)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralHexadecimalInteger) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "0xf")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            static_cast<int64_t>(15)
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralFloat) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::FLOAT, "6.28")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            6.28
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralString) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::STRING, R"("hello \n \\ \"wrld\" ")")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            std::string("hello \n \\ \"wrld\" ")
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralBoolean) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            true
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, ListExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, BRACKET_OPEN),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::INTEGER, "2"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::INTEGER, "3"),
            Token(Token::Type::OPERATOR, BRACKET_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::List(
            {
                new AstNode::Expression::Literal(static_cast<int64_t>(1)),
                new AstNode::Expression::Literal(static_cast<int64_t>(2)),
                new AstNode::Expression::Literal(static_cast<int64_t>(3))
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryAddition) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "2"),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION),
            Token(Token::Type::INTEGER, "3")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ARITHMETIC_ADDITION,
            new AstNode::Expression::Literal(
                static_cast<int64_t>(2)
            ),
            new AstNode::Expression::Literal(
                static_cast<int64_t>(3)
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryAssignment) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "10"),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ASSIGNMENT,
            new AstNode::Expression::Access("a"),
            new AstNode::Expression::Literal(static_cast<int64_t>(10))
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryCompoundAssignment) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ASSIGNMENT_ADDITION),
            Token(Token::Type::INTEGER, "10"),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ASSIGNMENT_ADDITION,
            new AstNode::Expression::Access("a"),
            new AstNode::Expression::Literal(static_cast<int64_t>(10))
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, ComplexBinaryExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION),
            Token(Token::Type::INTEGER, "2"),
            Token(Token::Type::OPERATOR, ARITHMETIC_MULTIPLICATION),
            Token(Token::Type::INTEGER, "3")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            ARITHMETIC_ADDITION,
            new AstNode::Expression::Literal(static_cast<int64_t>(1)),
            new AstNode::Expression::Binary(
                ARITHMETIC_MULTIPLICATION,
                new AstNode::Expression::Literal(static_cast<int64_t>(2)),
                new AstNode::Expression::Literal(static_cast<int64_t>(3))
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, GroupedBinaryExpression) {
        // (a + b) * c
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION),
            Token(Token::Type::IDENTIFIER, "b"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, ARITHMETIC_MULTIPLICATION),
            Token(Token::Type::IDENTIFIER, "c")
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
            Token(Token::Type::OPERATOR, UNARY_NEGATIVE),
            Token(Token::Type::INTEGER, "5")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            UNARY_NEGATIVE,
            new AstNode::Expression::Literal(
                static_cast<int64_t>(5)
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, PostfixUnaryExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, UNARY_INCREMENT)
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
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::INTEGER, "7"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Group(
            new AstNode::Expression::Literal(
                static_cast<int64_t>(7)
            )
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, FunctionCallExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "foo"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::INTEGER, "42"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::FLOAT, "3.14"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Function(
            "foo",
            {
                new AstNode::Expression::Literal(static_cast<int64_t>(42)),
                new AstNode::Expression::Literal(3.14)
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, NestedFunctionCallExpression) {
        // foo(bar(1), baz(2))
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "foo"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
                // First argument: bar(1)
                Token(Token::Type::IDENTIFIER, "bar"),
                Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
                Token(Token::Type::INTEGER, "1"),
                Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
                Token(Token::Type::OPERATOR, COMMA),
                // Second argument: baz(2)
                Token(Token::Type::IDENTIFIER, "baz"),
                Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
                Token(Token::Type::INTEGER, "2"),
                Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Function(
            "foo",
            {
                new AstNode::Expression::Function("bar", { new AstNode::Expression::Literal(static_cast<int64_t>(1)) }),
                new AstNode::Expression::Function("baz", { new AstNode::Expression::Literal(static_cast<int64_t>(2)) })
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, MethodCallExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "obj"),
            Token(Token::Type::OPERATOR, MEMBER_ACCESS),
            Token(Token::Type::IDENTIFIER, "method"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE)
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
            Token(Token::Type::IDENTIFIER, "obj"),
            Token(Token::Type::OPERATOR, MEMBER_ACCESS),
            Token(Token::Type::IDENTIFIER, "method"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::INTEGER, "10"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::INTEGER, "20"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Method(
            "obj", "method",
            {
                new AstNode::Expression::Literal(static_cast<int64_t>(10)),
                new AstNode::Expression::Literal(static_cast<int64_t>(20))
            }
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, MemberAccessExpression) {
        // obj.field
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "obj"),
            Token(Token::Type::OPERATOR, MEMBER_ACCESS),
            Token(Token::Type::IDENTIFIER, "field")
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "obj", "field"
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, AccessSubscript) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "arr"),
            Token(Token::Type::OPERATOR, BRACKET_OPEN),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, BRACKET_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "arr",
            new AstNode::Expression::Literal(static_cast<int64_t>(0))
        ));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, UnmatchedParenthesis) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::INTEGER, "42")
        };
        EXPECT_THROW(TEST_PARSE_EXPRESSION(sampleTokens, nullptr), SyntaxError);
    }

    GROUP_TEST_F(ParserTest, StatementTests, ExpressionStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "foo"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::INTEGER, "42"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Expression(
            new AstNode::Expression::Function("foo", { new AstNode::Expression::Literal(static_cast<int64_t>(42)) })
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }
    
    GROUP_TEST_F(ParserTest, StatementTests, DeclarationStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "10"),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            {},
            new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
            new AstNode::Expression::Literal(static_cast<int64_t>(10))
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, VirtualDeclarationStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_VIRTUAL),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "10"),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            {"virtual"},
            new AstNode::Specifier::Type(PRIMITIVE_INT, {}),
            new AstNode::Expression::Literal(static_cast<int64_t>(10))
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ReturnStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::INTEGER, "42"),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Return(
            new AstNode::Expression::Literal(static_cast<int64_t>(42))
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ReturnVoidStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::OPERATOR, SEMICOLON)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Return(
            std::nullopt
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, IfElseStatement) {
        // if ( a ) { b = 1; } else { b = 2; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_IF),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "b"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            Token(Token::Type::IDENTIFIER, RESERVED_ELSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "b"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "2"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };

        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Access("a"),
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("b"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(1))
                ))
            },
            {},
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("b"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(2))
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, IfElseIfStatement) {
        // if (a) { x = 1; } else if (b) { x = 2; } else { x = 3; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_IF),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            Token(Token::Type::IDENTIFIER, RESERVED_ELSE),
            Token(Token::Type::IDENTIFIER, RESERVED_IF),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, "b"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "2"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            Token(Token::Type::IDENTIFIER, RESERVED_ELSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "3"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Access("a"),
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("x"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(1))
                ))
            },
            {
                new AstNode::Statement::If(
                    new AstNode::Expression::Access("b"),
                    {
                        new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                            ASSIGNMENT,
                            new AstNode::Expression::Access("x"),
                            new AstNode::Expression::Literal(static_cast<int64_t>(2))
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
                    new AstNode::Expression::Literal(static_cast<int64_t>(3))
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, WhileStatement) {
        // while ( a ) { a = a - 1; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_WHILE),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ARITHMETIC_SUBTRACTION),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
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
                        new AstNode::Expression::Literal(static_cast<int64_t>(1))
                    )
                ))
            }
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ForStatement) {
        // for ( int i = 0; i < 10; i++ ) { sum = sum + i; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_FOR),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            // Init: int i = 0;
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "i"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            // Condition: i < 10;
            Token(Token::Type::IDENTIFIER, "i"),
            Token(Token::Type::OPERATOR, COMPARISON_LT),
            Token(Token::Type::INTEGER, "10"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            // Increment: i++
            Token(Token::Type::IDENTIFIER, "i"),
            Token(Token::Type::OPERATOR, UNARY_INCREMENT),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            // Block: sum = sum + i;
            Token(Token::Type::IDENTIFIER, "sum"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::IDENTIFIER, "sum"),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION),
            Token(Token::Type::IDENTIFIER, "i"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            new AstNode::Statement::Declaration(
                "i",
            {},
                new AstNode::Specifier::Type(
                    PRIMITIVE_INT,
                    {}
                ),
                new AstNode::Expression::Literal(static_cast<int64_t>(0))
            ),
            new AstNode::Statement::Expression(
                new AstNode::Expression::Binary(
                    COMPARISON_LT,
                    new AstNode::Expression::Access("i"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(10))
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
            Token(Token::Type::IDENTIFIER, RESERVED_FOR),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            // Empty init statement
            Token(Token::Type::OPERATOR, SEMICOLON),
            // Empty condition
            Token(Token::Type::OPERATOR, SEMICOLON),
            // Empty increment expression, then closing parenthesis
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
                // Block: foo();
                Token(Token::Type::IDENTIFIER, "foo"),
                Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
                Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
                Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
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
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "5")
        };
        EXPECT_THROW(TEST_PARSE_STATEMENT(sampleTokens, nullptr), SyntaxError);
    }


    GROUP_TEST_F(ParserTest, FunctionTests, Procedure) {
        // int main() { return 0; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "main"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
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
                    new AstNode::Expression::Literal(static_cast<int64_t>(0))
                )
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, ProcedureWithParameters) {
        // Procedure with parameters: int add(int a, int b) { return a + b; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "add"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "b"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION),
            Token(Token::Type::IDENTIFIER, "b"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
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

    GROUP_TEST_F(ParserTest, FunctionTests, Task) {
        // task foo() { a = 1; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_TASK),
            Token(Token::Type::IDENTIFIER, "foo"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Task(
            "foo",
            {},
            {},
            {},
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("a"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(1))
                ))
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, TaskWithInitializerOptions) {
        // task foo(LIGHT L1, LIGHT L2, LIGHT L3) : triggerOn([L1, L2], L3), dropRead { }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_TASK),
            Token(Token::Type::IDENTIFIER, "foo"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::IDENTIFIER, "LIGHT"),
            Token(Token::Type::IDENTIFIER, "L1"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, "LIGHT"),
            Token(Token::Type::IDENTIFIER, "L2"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, "LIGHT"),
            Token(Token::Type::IDENTIFIER, "L3"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, COLON),
            Token(Token::Type::IDENTIFIER, "triggerOn"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, BRACKET_OPEN),
            Token(Token::Type::IDENTIFIER, "L1"),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, "L2"),
            Token(Token::Type::OPERATOR, BRACKET_CLOSE),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, "L3"),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, COMMA),
            Token(Token::Type::IDENTIFIER, "dropRead"),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Task(
            "foo",
            {
                new AstNode::Specifier::Type(
                    "LIGHT",
                    {}
                ),
                new AstNode::Specifier::Type(
                    "LIGHT",
                    {}
                ),
                new AstNode::Specifier::Type(
                    "LIGHT",
                    {}
                )
            },
            {
                "L1",
                "L2",
                "L3"
            },
            {
                new AstNode::Initializer::Task(
                    "triggerOn",
                    {
                        new AstNode::Expression::List(
                            {
                                new AstNode::Expression::Access(
                                    "L1"
                                ),
                                new AstNode::Expression::Access(
                                    "L2"
                                )
                            }
                        ),
                        new AstNode::Expression::Access(
                            "L3"
                        ),
                    }
                ),
                new AstNode::Initializer::Task(
                    "dropRead",
                    {}
                )
            },
            {}
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, TaskMultipleStatements) {
        // Task with multiple statements: task init() { x = 0; y = 0; }
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_TASK),
            Token(Token::Type::IDENTIFIER, "init"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::IDENTIFIER, "y"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };
        auto expectedAst = std::unique_ptr<AstNode>(new AstNode::Function::Task(
            "init",
            {},
            {},
            {},
            {
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("x"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(0))
                )),
                new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                    ASSIGNMENT,
                    new AstNode::Expression::Access("y"),
                    new AstNode::Expression::Literal(static_cast<int64_t>(0))
                ))
            }
        ));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, FunctionTests, MissingFunctionName) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
        };
        EXPECT_THROW(TEST_PARSE_FUNCTION(sampleTokens, nullptr), SyntaxError);
    }

    GROUP_TEST_F(ParserTest, SourceTests, FullSource) {
        // Full source: one procedure, one task, and one setup.
        // Procedure: int main() { return 0; }
        // Task: task helper() { a = 1; }
        // Setup: setup() { int x = 5; }
        std::vector<Token> sampleTokens {
            // Procedure:
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "main"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            // Task:
            Token(Token::Type::IDENTIFIER, RESERVED_TASK),
            Token(Token::Type::IDENTIFIER, "helper"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "a"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            // Setup:
            Token(Token::Type::IDENTIFIER, RESERVED_SETUP),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "x"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::INTEGER, "5"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
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
                            new AstNode::Expression::Literal(static_cast<int64_t>(0))
                        )
                    }
                )
            },
            {
                new AstNode::Function::Task(
                    "helper",
                    {},
                    {},
                    {},
                    {
                        new AstNode::Statement::Expression(new AstNode::Expression::Binary(
                            ASSIGNMENT,
                            new AstNode::Expression::Access("a"),
                            new AstNode::Expression::Literal(static_cast<int64_t>(1))
                        ))
                    }
                )
            },
            new AstNode::Setup(
                {
                    new AstNode::Statement::Declaration(
                        "x",
                        {},
                        new AstNode::Specifier::Type(
                            PRIMITIVE_INT,
                            {}
                        ),
                        new AstNode::Expression::Literal(static_cast<int64_t>(5))
                    )
                }
            )
        ));
        TEST_PARSE_SOURCE(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SourceTests, MultipleProceduresAndTasks) {
        std::vector<Token> sampleTokens {
            // Procedure 1: int main() { return 0; }
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "main"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::INTEGER, "0"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            // Procedure 2: int helper() { return 1; }
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT),
            Token(Token::Type::IDENTIFIER, "helper"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN),
            Token(Token::Type::INTEGER, "1"),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            // Task: task config() { flag = true; }
            Token(Token::Type::IDENTIFIER, RESERVED_TASK),
            Token(Token::Type::IDENTIFIER, "config"),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::IDENTIFIER, "flag"),
            Token(Token::Type::OPERATOR, ASSIGNMENT),
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE),
            Token(Token::Type::OPERATOR, SEMICOLON),
            Token(Token::Type::OPERATOR, BRACE_CLOSE),
            // Setup: setup() { }
            Token(Token::Type::IDENTIFIER, RESERVED_SETUP),
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE),
            Token(Token::Type::OPERATOR, BRACE_OPEN),
            Token(Token::Type::OPERATOR, BRACE_CLOSE)
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
                            new AstNode::Expression::Literal(static_cast<int64_t>(0))
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
                            new AstNode::Expression::Literal(static_cast<int64_t>(1))
                        )
                    }
                )
            },
            {
                new AstNode::Function::Task(
                    "config",
                    {},
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
            Token(Token::Type::OPERATOR, "?")
        };
        EXPECT_THROW(TEST_PARSE_SOURCE(sampleTokens, nullptr), SyntaxError);
    }

}