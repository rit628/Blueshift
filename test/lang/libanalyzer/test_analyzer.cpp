#include "ast.hpp"
#include "error_types.hpp"
#include "fixtures/analyzer_test.hpp"
#include "include/Common.hpp"
#include "include/reserved_tokens.hpp"
#include "test_macros.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <variant>

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
        expectedMetadata.literalPool = {
            {2, 0}
        };

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

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidTypename) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                "faketype",
                {}
            )
            ,
            new AstNode::Expression::Literal(
                "string"
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, ContainerWithNoArgs) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                "list",
                {}
            )
            ,
            new AstNode::Expression::Literal(
                "string"
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, PrimitiveWithArgs) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                "int",
                {
                    new AstNode::Specifier::Type(
                        "int",
                        {}
                    )
                }
            )
            ,
            new AstNode::Expression::Literal(
                "string"
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, ListWithTwoArgs) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                "list",
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
            )
            ,
            new AstNode::Expression::Literal(
                "string"
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, MapWithOneArg) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                "map",
                {
                    new AstNode::Specifier::Type(
                        "int",
                        {}
                    )
                }
            )
            ,
            new AstNode::Expression::Literal(
                "string"
            )
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidKeyType) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            new AstNode::Specifier::Type(
                "map",
                {
                    new AstNode::Specifier::Type(
                        "list",
                        {
                            new AstNode::Specifier::Type(
                                "int",
                                {}
                            )
                        }
                    ),
                    new AstNode::Specifier::Type(
                        "int",
                        {}
                    )
                }
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

        Metadata expectedMetadata;
        expectedMetadata.literalPool = {
            {std::monostate(), 0}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
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

        Metadata expectedMetadata;
        expectedMetadata.literalPool = {
            {std::monostate(), 0}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
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
        expectedMetadata.literalPool = {
            {0, 0},
            {10, 1}
        };

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
        expectedMetadata.literalPool = {
            {0, 0},
            {10.23, 1}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidProcedureArgument) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "g",
                    new AstNode::Specifier::Type(
                        "void",
                        {}
                    ),
                    {
                        new AstNode::Specifier::Type(
                            "int",
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            "string",
                            {}
                        )
                    },
                    {
                        "arg1",
                        "arg2"
                    },
                    {}
                ),
                new AstNode::Function::Procedure(
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
                                {
                                    new AstNode::Expression::Literal(
                                        std::string("not an int")
                                    ),
                                    new AstNode::Expression::Literal(
                                        int64_t(7)
                                    )
                                }
                            )
                        )
                    }
                )
            },
            {},
            {}
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, HeterogenousListLiteral) {
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
                    new AstNode::Expression::List(
                        {
                            new AstNode::Expression::Literal(
                                int64_t(4)
                            ),
                            new AstNode::Expression::Literal(
                                std::string("not an int")
                            )
                        }
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, HeterogenousMapLiteral) {
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
                    new AstNode::Expression::Map(
                        {
                            {
                                new AstNode::Expression::Literal(
                                    std::string("key1")
                                ),
                                new AstNode::Expression::Literal(
                                    int64_t(20)
                                )
                            },
                            {
                                new AstNode::Expression::Literal(
                                    std::string("key2")
                                ),
                                new AstNode::Expression::Literal(
                                    bool(false)
                                )
                            }
                        }
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidMapKey) {
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
                    new AstNode::Expression::Map(
                        {
                            {
                                new AstNode::Expression::List(
                                    {
                                        new AstNode::Expression::Literal(
                                            int64_t(10)
                                        )
                                    }
                                ),
                                new AstNode::Expression::Literal(
                                    int64_t(20)
                                )
                            }
                        }
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
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

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidArgumentCount) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "g",
                    new AstNode::Specifier::Type(
                        "void",
                        {}
                    ),
                    {
                        new AstNode::Specifier::Type(
                            "int",
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            "string",
                            {}
                        )
                    },
                    {
                        "arg1",
                        "arg2"
                    },
                    {}
                ),
                new AstNode::Function::Procedure(
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
                                {
                                    new AstNode::Expression::Literal(
                                        int64_t(2)
                                    )
                                }
                            )
                        )
                    }
                )
            },
            {},
            {}
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

    GROUP_TEST_F(AnalyzerTest, ContextTests, ContinueInWhile) {
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
        expectedMetadata.literalPool = {
            {true, 0}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, ContextTests, BreakInWhile) {
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
        expectedMetadata.literalPool = {
            {true, 0}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, ContextTests, ContinueInFor) {
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

    GROUP_TEST_F(AnalyzerTest, ContextTests, BreakInFor) {
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

    GROUP_TEST_F(AnalyzerTest, ConfigTests, OblockConfig) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Oblock(
            "foo",
            {
                new AstNode::Specifier::Type(
                    "LINE_WRITER",
                    {}
                ),
                new AstNode::Specifier::Type(
                    "LINE_WRITER",
                    {}
                ),
                new AstNode::Specifier::Type(
                    "LINE_WRITER",
                    {}
                )
            },
            {
                "L1",
                "L2",
                "L3"
            },
            {
                new AstNode::Initializer::Oblock(
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
                new AstNode::Initializer::Oblock(
                    "dropRead",
                    {}
                )
            },
            {}
        ));
    
        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Function::Oblock(
            "foo",
            {
                new AstNode::Specifier::Type(
                    "LINE_WRITER",
                    {}
                ),
                new AstNode::Specifier::Type(
                    "LINE_WRITER",
                    {}
                ),
                new AstNode::Specifier::Type(
                    "LINE_WRITER",
                    {}
                )
            },
            {
                "L1",
                "L2",
                "L3"
            },
            {
                new AstNode::Initializer::Oblock(
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
                new AstNode::Initializer::Oblock(
                    "dropRead",
                    {}
                )
            },
            {}
        ));

        Metadata expectedMetadata;

        expectedMetadata.oblockDescriptors = {
            {"foo", OBlockDesc{
                "foo",
                {},
                0,
                true,
                false,
                {
                    {"L1", "L2"},
                    {"L3"}
                },
                true
            }}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

}