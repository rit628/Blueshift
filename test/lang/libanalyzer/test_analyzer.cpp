#include "ast.hpp"
#include "error_types.hpp"
#include "fixtures/analyzer_test.hpp"
#include "include/reserved_tokens.hpp"
#include "test_macros.hpp"
#include <gtest/gtest.h>
#include <memory>

namespace BlsLang {

    GROUP_TEST_F(AnalyzerTest, TypeTests, BadDeclaration) {
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

}