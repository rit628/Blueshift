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

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidIfCondition) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Literal(
                std::string("true")
            ),
            {},
            {},
            {}
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidForCondition) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            new AstNode::Statement::Expression(
                new AstNode::Expression::Literal(
                    double(6.28)
                )
            ),
            std::nullopt,
            {}
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidReturnValue) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "string",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Literal(
                        int64_t(25)
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidVoidReturn) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "string",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    std::nullopt
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, VoidReturn) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    std::nullopt
                )
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    std::nullopt
                )
            }
        ));

        TEST_ANALYZE(ast, decoratedAst);
    }

    /* NEEDS DEPENDENCY GRAPH */
    GROUP_TEST_F(AnalyzerTest, TypeTests, VoidNoReturn) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {}
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {}
        ));

        TEST_ANALYZE(ast, decoratedAst);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, TypedReturn) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "int",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Literal(
                        int64_t(10)
                    )
                )
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "int",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Literal(
                        int64_t(10)
                    )
                )
            }
        ));

        Metadata expectedMetadata;
        expectedMetadata.literalPool.emplace(10, 2);

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    /* NEEDS DEPENDENCY GRAPH */
    // GROUP_TEST_F(AnalyzerTest, TypeTests, TypedNoReturn) {
    //     auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
    //         "f",
    //         new AstNode::Specifier::Type(
    //             "int",
    //             {}
    //         ),
    //         {},
    //         {},
    //         {}
    //     ));

    //     EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    // }
    
    GROUP_TEST_F(AnalyzerTest, TypeTests, ImplicitReturnConversion) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "int",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Literal(
                        double(10.23)
                    )
                )
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "int",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Literal(
                        double(10.23)
                    )
                )
            }
        ));

        Metadata expectedMetadata;
        expectedMetadata.literalPool.emplace(10.23, 2);

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidWhileCondition) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Binary(
                "-",
                new AstNode::Expression::Literal(
                    int64_t(10)
                ),
                new AstNode::Expression::Literal(
                    int64_t(9)
                )
            ),
            {}
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

    GROUP_TEST_F(AnalyzerTest, LogicTests, MethodOnUndeclaredVariable) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Expression(
            new AstNode::Expression::Method(
                "x",
                "size",
                {}
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, MethodOnInteger) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    new AstNode::Specifier::Type(
                        "int",
                        {}
                    ),
                    std::nullopt
                ),
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Method(
                        "x",
                        "size",
                        {}
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, MapMethodOnList) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    new AstNode::Specifier::Type(
                        "list",
                        {
                            new AstNode::Specifier::Type(
                                "int",
                                {}
                            )
                        }
                    ),
                    std::nullopt
                ),
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Method(
                        "x",
                        "add",
                        {}
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, ListMethodOnMap) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    new AstNode::Specifier::Type(
                        "map",
                        {
                            new AstNode::Specifier::Type(
                                "int",
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                "int",
                                {}
                            )
                        }
                    ),
                    std::nullopt
                ),
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Method(
                        "x",
                        "append",
                        {}
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidMethod) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    new AstNode::Specifier::Type(
                        "map",
                        {
                            new AstNode::Specifier::Type(
                                "int",
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                "int",
                                {}
                            )
                        }
                    ),
                    std::nullopt
                ),
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Method(
                        "x",
                        "INVALID_METHOD",
                        {}
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    /* TODO: Add tests checking proper argument adherence once new method system is established */
    
    GROUP_TEST_F(AnalyzerTest, LogicTests, UndefinedProcedureCall) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                "void",
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Function(
                        "g",
                        {}
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, ContextTests, InvalidContinue) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Continue());
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, ContextTests, InvalidBreak) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Break());
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, ContextTests, ContinueInIf) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Literal(
                bool(true)
            ),
            {
                new AstNode::Statement::Continue()
            },
            {},
            {}
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, ContextTests, BreakInIf) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Literal(
                bool(true)
            ),
            {
                new AstNode::Statement::Break()
            },
            {},
            {}
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, ContinueInWhile) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Continue()
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Continue()
            }
        ));

        Metadata expectedMetadata;
        expectedMetadata.literalPool.emplace(true, 2);

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, BreakInWhile) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Break()
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Break()
            }
        ));

        Metadata expectedMetadata;
        expectedMetadata.literalPool.emplace(true, 2);

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, ContinueInFor) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {
                new AstNode::Statement::Continue()
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {
                new AstNode::Statement::Continue()
            }
        ));

        Metadata expectedMetadata;

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, BreakInFor) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {
                new AstNode::Statement::Break()
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {
                new AstNode::Statement::Break()
            }
        ));

        Metadata expectedMetadata;

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

}