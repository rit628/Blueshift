#include "ast.hpp"
#include "fixtures/parser_test.hpp"
#include "test_macros.hpp"
#include "include/reserved_tokens.hpp"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

namespace BlsLang {

    // ─── EXPRESSION TESTS ─────────────────────────────────────────────

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralInteger) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "123", 0, 1, 1)
        };
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Literal>((size_t)123);
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralFloat) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::FLOAT, "3.14", 0, 1, 1)
        };
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Literal>(3.14);
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralString) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::STRING, "\"hello\"", 0, 1, 1)
        };
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Literal>(std::string("\"hello\""));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, LiteralBoolean) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, LITERAL_TRUE, 0, 1, 1)
        };
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Literal>(true);
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, BinaryAddition) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::INTEGER, "2", 0, 1, 1),
            Token(Token::Type::OPERATOR, ARITHMETIC_ADDITION, 1, 1, 2),
            Token(Token::Type::INTEGER, "3", 2, 1, 3)
        };
        auto left = std::make_unique<AstNode::Expression::Literal>((size_t)2);
        auto right = std::make_unique<AstNode::Expression::Literal>((size_t)3);
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Binary>(ARITHMETIC_ADDITION, std::move(left), std::move(right));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, UnaryNegative) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, UNARY_NEGATIVE, 0, 1, 1),
            Token(Token::Type::INTEGER, "5", 1, 1, 2)
        };
        auto literal = std::make_unique<AstNode::Expression::Literal>((size_t)5);
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Unary>(UNARY_NEGATIVE, std::move(literal));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, GroupedExpression) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::OPERATOR, PARENTHESES_OPEN, 0, 1, 1),
            Token(Token::Type::INTEGER, "7", 1, 1, 2),
            Token(Token::Type::OPERATOR, PARENTHESES_CLOSE, 2, 1, 3)
        };
        auto inner = std::make_unique<AstNode::Expression::Literal>((size_t)7);
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Group>(std::move(inner));
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
        std::vector<std::unique_ptr<AstNode::Expression>> args;
        args.push_back(std::make_unique<AstNode::Expression::Literal>((size_t)42));
        args.push_back(std::make_unique<AstNode::Expression::Literal>(3.14));
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Function>("foo", std::move(args));
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
        std::vector<std::unique_ptr<AstNode::Expression>> args;
        args.push_back(std::make_unique<AstNode::Expression::Literal>(true));
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Method>("obj", "method", std::move(args));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, ExpressionTests, AccessSubscript) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "arr", 0, 1, 1),
            Token(Token::Type::OPERATOR, BRACKET_OPEN, 1, 1, 2),
            Token(Token::Type::INTEGER, "0", 2, 1, 3),
            Token(Token::Type::OPERATOR, BRACKET_CLOSE, 3, 1, 4)
        };
        auto subscript = std::make_unique<AstNode::Expression::Literal>((size_t)0);
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Expression::Access>("arr", std::move(subscript));
        TEST_PARSE_EXPRESSION(sampleTokens, std::move(expectedAst));
    }

    // ─── STATEMENT TESTS ──────────────────────────────────────────────

    GROUP_TEST_F(ParserTest, StatementTests, AssignmentStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a", 0, 1, 1),
            Token(Token::Type::OPERATOR, ASSIGNMENT, 1, 1, 2),
            Token(Token::Type::INTEGER, "10", 2, 1, 3),
            Token(Token::Type::OPERATOR, SEMICOLON, 3, 1, 4)
        };
        auto recipient = std::make_unique<AstNode::Expression::Access>("a");
        auto value = std::make_unique<AstNode::Expression::Literal>((size_t)10);
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(value));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ReturnStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 0, 1, 1),
            Token(Token::Type::INTEGER, "42", 1, 1, 2),
            Token(Token::Type::OPERATOR, SEMICOLON, 2, 1, 3)
        };
        auto value = std::make_unique<AstNode::Expression::Literal>((size_t)42);
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Statement::Return>(std::make_optional(std::move(value)));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, ReturnVoidStatement) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, RESERVED_RETURN, 0, 1, 1),
            Token(Token::Type::OPERATOR, SEMICOLON, 1, 1, 2)
        };
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Statement::Return>(std::nullopt);
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
        auto condition = std::make_unique<AstNode::Expression::Access>("a");
        std::vector<std::unique_ptr<AstNode::Statement>> ifBlock;
        {
            auto recipient = std::make_unique<AstNode::Expression::Access>("b");
            auto value = std::make_unique<AstNode::Expression::Literal>((size_t)1);
            ifBlock.push_back(std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(value)));
        }
        std::vector<std::unique_ptr<AstNode::Statement>> elseBlock;
        {
            auto recipient = std::make_unique<AstNode::Expression::Access>("b");
            auto value = std::make_unique<AstNode::Expression::Literal>((size_t)2);
            elseBlock.push_back(std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(value)));
        }
        std::vector<std::unique_ptr<AstNode::Statement::If>> elseIf;
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Statement::If>(std::move(condition), std::move(ifBlock), std::move(elseIf), std::move(elseBlock));
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
        auto condition = std::make_unique<AstNode::Expression::Access>("a");
        std::vector<std::unique_ptr<AstNode::Statement>> block;
        {
            auto recipient = std::make_unique<AstNode::Expression::Access>("a");
            auto left = std::make_unique<AstNode::Expression::Access>("a");
            auto right = std::make_unique<AstNode::Expression::Literal>((size_t)1);
            auto binary = std::make_unique<AstNode::Expression::Binary>(ARITHMETIC_SUBTRACTION, std::move(left), std::move(right));
            block.push_back(std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(binary)));
        }
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Statement::While>(std::move(condition), std::move(block));
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
        // Build init statement as a declaration: int i = 0
        auto initType = std::make_unique<AstNode::Specifier::Type>(PRIMITIVE_INT, std::vector<std::unique_ptr<AstNode::Specifier::Type>>{});
        auto initValue = std::make_unique<AstNode::Expression::Literal>((size_t)0);
        std::unique_ptr<AstNode::Statement> initStmt =
            std::make_unique<AstNode::Statement::Declaration>("i", std::move(initType), std::make_optional(std::move(initValue)));
        // Condition: i < 10
        auto condLeft = std::make_unique<AstNode::Expression::Access>("i");
        auto condRight = std::make_unique<AstNode::Expression::Literal>((size_t)10);
        std::unique_ptr<AstNode::Statement::Expression> condition = std::make_unique<AstNode::Statement::Expression>(
            std::make_unique<AstNode::Expression::Binary>(COMPARISON_LT, std::move(condLeft), std::move(condRight)));
        // Increment: i++ (postfix)
        auto primary = std::make_unique<AstNode::Expression::Access>("i");
        std::unique_ptr<AstNode::Expression> incExpr =
            std::make_unique<AstNode::Expression::Unary>(UNARY_INCREMENT, std::move(primary), false);
        // Block: sum = sum + i;
        std::vector<std::unique_ptr<AstNode::Statement>> block;
        {
            auto recipient = std::make_unique<AstNode::Expression::Access>("sum");
            auto leftExpr = std::make_unique<AstNode::Expression::Access>("sum");
            auto rightExpr = std::make_unique<AstNode::Expression::Access>("i");
            auto addExpr = std::make_unique<AstNode::Expression::Binary>(ARITHMETIC_ADDITION, std::move(leftExpr), std::move(rightExpr));
            block.push_back(std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(addExpr)));
        }
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Statement::For>(
                std::make_optional(std::move(initStmt)),
                std::make_optional(std::move(condition)),
                std::make_optional(std::move(incExpr)),
                std::move(block)
            );
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    // ─── FUNCTION TESTS ───────────────────────────────────────────────

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
        auto returnType = std::make_unique<AstNode::Specifier::Type>(PRIMITIVE_INT, std::vector<std::unique_ptr<AstNode::Specifier::Type>>{});
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> paramTypes;
        std::vector<std::string> params;
        std::vector<std::unique_ptr<AstNode::Statement>> statements;
        {
            auto retExpr = std::make_unique<AstNode::Expression::Literal>((size_t)0);
            statements.push_back(std::make_unique<AstNode::Statement::Return>(std::make_optional(std::move(retExpr))));
        }
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Function::Procedure>("main", std::make_optional(std::move(returnType)), std::move(paramTypes), std::move(params), std::move(statements));
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
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> paramTypes;
        std::vector<std::string> params;
        std::vector<std::unique_ptr<AstNode::Statement>> statements;
        {
            auto recipient = std::make_unique<AstNode::Expression::Access>("a");
            auto value = std::make_unique<AstNode::Expression::Literal>((size_t)1);
            statements.push_back(std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(value)));
        }
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Function::Oblock>("foo", std::move(paramTypes), std::move(params), std::move(statements));
        TEST_PARSE_FUNCTION(sampleTokens, std::move(expectedAst));
    }

    // ─── SOURCE TESTS ─────────────────────────────────────────────────

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
        // Build procedure AST
        auto procReturnType = std::make_unique<AstNode::Specifier::Type>(PRIMITIVE_INT, std::vector<std::unique_ptr<AstNode::Specifier::Type>>{});
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> procParamTypes;
        std::vector<std::string> procParams;
        std::vector<std::unique_ptr<AstNode::Statement>> procStatements;
        {
            auto retExpr = std::make_unique<AstNode::Expression::Literal>((size_t)0);
            procStatements.push_back(std::make_unique<AstNode::Statement::Return>(std::make_optional(std::move(retExpr))));
        }
        auto procedure = std::make_unique<AstNode::Function::Procedure>("main", std::make_optional(std::move(procReturnType)), std::move(procParamTypes), std::move(procParams), std::move(procStatements));
        // Build oblock AST
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> oblockParamTypes;
        std::vector<std::string> oblockParams;
        std::vector<std::unique_ptr<AstNode::Statement>> oblockStatements;
        {
            auto recipient = std::make_unique<AstNode::Expression::Access>("a");
            auto value = std::make_unique<AstNode::Expression::Literal>((size_t)1);
            oblockStatements.push_back(std::make_unique<AstNode::Statement::Assignment>(std::move(recipient), std::move(value)));
        }
        auto oblock = std::make_unique<AstNode::Function::Oblock>("helper", std::move(oblockParamTypes), std::move(oblockParams), std::move(oblockStatements));
        // Build setup AST
        std::vector<std::unique_ptr<AstNode::Statement>> setupStatements;
        {
            auto type = std::make_unique<AstNode::Specifier::Type>(PRIMITIVE_INT, std::vector<std::unique_ptr<AstNode::Specifier::Type>>{});
            auto initValue = std::make_unique<AstNode::Expression::Literal>((size_t)5);
            setupStatements.push_back(std::make_unique<AstNode::Statement::Declaration>("x", std::move(type), std::make_optional(std::move(initValue))));
        }
        auto setup = std::make_unique<AstNode::Setup>(std::move(setupStatements));
        // Build full source AST
        std::vector<std::unique_ptr<AstNode::Function>> procedures;
        procedures.push_back(std::move(procedure));
        std::vector<std::unique_ptr<AstNode::Function>> oblocks;
        oblocks.push_back(std::move(oblock));
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Source>(std::move(procedures), std::move(oblocks), std::move(setup));
        TEST_PARSE_SOURCE(sampleTokens, std::move(expectedAst));
    }

    // ─── SPECIFIER TESTS ──────────────────────────────────────────────

    GROUP_TEST_F(ParserTest, SpecifierTests, SimpleType) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 0, 1, 1)
        };
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Specifier::Type>(PRIMITIVE_INT, std::vector<std::unique_ptr<AstNode::Specifier::Type>>{});
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, SpecifierTests, ContainerType) {
        // Example: array<int>
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "array", 0, 1, 1),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_OPEN, 1, 1, 2),
            Token(Token::Type::IDENTIFIER, PRIMITIVE_INT, 2, 1, 3),
            Token(Token::Type::OPERATOR, TYPE_DELIMITER_CLOSE, 3, 1, 4)
        };
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> typeArgs;
        typeArgs.push_back(std::make_unique<AstNode::Specifier::Type>(PRIMITIVE_INT, std::vector<std::unique_ptr<AstNode::Specifier::Type>>{}));
        std::unique_ptr<AstNode> expectedAst =
            std::make_unique<AstNode::Specifier::Type>("array", std::move(typeArgs));
        TEST_PARSE_SPECIFIER(sampleTokens, std::move(expectedAst));
    }

}