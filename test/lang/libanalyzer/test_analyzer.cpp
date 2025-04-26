#include "ast.hpp"
#include "error_types.hpp"
#include "fixtures/analyzer_test.hpp"
#include "include/reserved_tokens.hpp"
#include "test_macros.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>

namespace BlsLang {
    
    GROUP_TEST_F(AnalyzerTest, TypeTests, ValidDeclaration) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {}
            )
            ,
            new AstNode::Expression::Literal(
                int64_t(2)
            )
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {}
            )
            ,
            new AstNode::Expression::Literal(
                int64_t(2)
            ),
            0
        ));

        Metadata expectedMetadata;
        expectedMetadata.literalPool.emplace(2, 2);

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidDeclaration) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {}
            )
            ,
            new AstNode::Expression::Literal(
                "string"
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Expression(
            new AstNode::Expression::Binary(
                "=",
                new AstNode::Expression::Literal(
                    int64_t(1)
                ),
                new AstNode::Expression::Literal(
                    int64_t(2)
                )
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidCompoundAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Expression(
            new AstNode::Expression::Binary(
                "*=",
                new AstNode::Expression::Literal(
                    int64_t(1)
                ),
                new AstNode::Expression::Literal(
                    int64_t(2)
                )
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidIncrement) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Expression(
            new AstNode::Expression::Unary(
                "++",
                new AstNode::Expression::Literal(
                    int64_t(1)
                )
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

}