#pragma once
#include "test_visitor.hpp"
#include "ast.hpp"
#include "parser.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace BlsLang {
    class ParserTest : public testing::Test {
        public:
            void TEST_PARSE_SOURCE(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseSource();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_FUNCTION(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseFunction();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_STATEMENT(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseStatement();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_EXPRESSION(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseExpression();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void TEST_PARSE_SPECIFIER(std::vector<Token> sampleTokens, std::unique_ptr<AstNode> expectedAst, bool success = true) {
                parser.ts.setStream(sampleTokens);
                auto ast = parser.parseSpecifier();
                checkAst(std::move(ast), std::move(expectedAst));
            }

            void checkAst(std::unique_ptr<AstNode> ast, std::unique_ptr<AstNode> expectedAst) {
                ASSERT_NE(ast, nullptr);
                ASSERT_NE(expectedAst, nullptr);
                Tester tester(std::move(expectedAst));
                ast->accept(tester);
            }

        private:
            Parser parser;
    };         
}