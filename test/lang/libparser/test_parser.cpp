#include "ast.hpp"
#include "fixtures/parser_test.hpp"
#include "test_macros.hpp"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>

namespace BlsLang {

    GROUP_TEST_F(ParserTest, StatementTests, First) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "a", 0, 1, 1),
            Token(Token::Type::OPERATOR, ";", 9, 1, 10),
        };
        std::unique_ptr<AstNode> expectedAst = std::make_unique<AstNode::Statement::Expression>(AstNode::Statement::Expression(
            std::make_unique<AstNode::Expression::Access>(
                "a"
            )
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

    GROUP_TEST_F(ParserTest, StatementTests, Second) {
        std::vector<Token> sampleTokens {
            Token(Token::Type::IDENTIFIER, "int", 0, 1, 1),
            Token(Token::Type::IDENTIFIER, "x", 4, 1, 5),
            Token(Token::Type::OPERATOR, "=", 6, 1, 7),
            Token(Token::Type::INTEGER, "5", 8, 1, 9),
            Token(Token::Type::OPERATOR, ";", 9, 1, 10),
        };
        std::unique_ptr<AstNode> expectedAst = std::make_unique<AstNode::Statement::Declaration>(AstNode::Statement::Declaration(
            "x",
            std::make_unique<AstNode::Specifier::Type>(AstNode::Specifier::Type(
                "int",
                {}
            )),
            std::make_unique<AstNode::Expression::Literal>(
                (size_t)5
            )
        ));
        TEST_PARSE_STATEMENT(sampleTokens, std::move(expectedAst));
    }

}