#include "ast.hpp"
#include "error_types.hpp"
#include "fixtures/analyzer_test.hpp"
#include "include/Common.hpp"
#include "include/reserved_tokens.hpp"
#include "libtype/bls_types.hpp"
#include "libtype/typedefs.hpp"
#include "test_macros.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <variant>

namespace BlsLang {
    
    GROUP_TEST_F(AnalyzerTest, TypeTests, ValidDeclaration) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            {},
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
            {},
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
            {},
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
            {},
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
            {},
            new AstNode::Specifier::Type(
                CONTAINER_LIST,
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
            {},
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
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
            {},
            new AstNode::Specifier::Type(
                CONTAINER_LIST,
                {
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
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
            {},
            new AstNode::Specifier::Type(
                CONTAINER_MAP,
                {
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
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
            {},
            new AstNode::Specifier::Type(
                CONTAINER_MAP,
                {
                    new AstNode::Specifier::Type(
                        CONTAINER_LIST,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            )
                        }
                    ),
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
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
                PRIMITIVE_STRING,
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

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidReturnValueFunction) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                PRIMITIVE_STRING,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Return(
                    new AstNode::Expression::Function(
                        "print",
                        {
                            new AstNode::Expression::Literal(
                                int64_t(12)
                            )
                        }
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
                PRIMITIVE_STRING,
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
                PRIMITIVE_VOID,
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

        auto decoratedAst = ast->clone();

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
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {}
        ));

        auto decoratedAst = ast->clone();

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
                PRIMITIVE_INT,
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

        auto decoratedAst = ast->clone();

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
    //             PRIMITIVE_INT,
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
                PRIMITIVE_INT,
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

        auto decoratedAst = ast->clone();

        Metadata expectedMetadata;
        expectedMetadata.literalPool = {
            {0, 0},
            {10.23, 1}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, ValidTrapCall) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Function(
            "print",
            {
                new AstNode::Expression::Literal(
                    std::string("arg1")
                ),
                new AstNode::Expression::Literal(
                    int64_t(2)
                )
            }
        ));

        auto decoratedAst = ast->clone();

        Metadata expectedMetadata;
        expectedMetadata.literalPool = {
            {"arg1", 0},
            {2, 1}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, TypeTests, InvalidProcedureArgument) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "g",
                    new AstNode::Specifier::Type(
                        PRIMITIVE_VOID,
                        {}
                    ),
                    {
                        new AstNode::Specifier::Type(
                            PRIMITIVE_INT,
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            PRIMITIVE_STRING,
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
                        PRIMITIVE_VOID,
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
                PRIMITIVE_VOID,
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
                PRIMITIVE_VOID,
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
                PRIMITIVE_VOID,
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
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
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
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_LIST,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_MAP,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_MAP,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidMethodArgumentCount) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_MAP,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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
                        {
                            new AstNode::Expression::Literal(
                                int64_t(1)
                            )
                        }
                    )
                )
            }
        ));
        EXPECT_THROW(TEST_ANALYZE(ast), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, LogicTests, InvalidMethodArgument) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_MAP,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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
                        {
                            new AstNode::Expression::Literal(
                                int64_t(1)
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

    GROUP_TEST_F(AnalyzerTest, LogicTests, ValidMethodInvocation) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_MAP,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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
                        {
                            new AstNode::Expression::Literal(
                                int64_t(1)
                            ),
                            new AstNode::Expression::Literal(
                                int64_t(6)
                            )
                        }
                    )
                )
            }
        ));

        auto decoratedAst = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                PRIMITIVE_VOID,
                {}
            ),
            {},
            {},
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        CONTAINER_MAP,
                        {
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
                                {}
                            ),
                            new AstNode::Specifier::Type(
                                PRIMITIVE_INT,
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
                        {
                            new AstNode::Expression::Literal(
                                int64_t(1)
                            ),
                            new AstNode::Expression::Literal(
                                int64_t(6)
                            )
                        },
                        0,
                        TYPE::map_t
                    )
                )
            }
        ));

        Metadata expectedMetadata;
        expectedMetadata.literalPool = {
            {std::monostate(), 0},
            {1, 1},
            {6, 2}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }
    
    GROUP_TEST_F(AnalyzerTest, LogicTests, UndefinedProcedureCall) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
            new AstNode::Specifier::Type(
                PRIMITIVE_VOID,
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
                        PRIMITIVE_VOID,
                        {}
                    ),
                    {
                        new AstNode::Specifier::Type(
                            PRIMITIVE_INT,
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            PRIMITIVE_STRING,
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
                        PRIMITIVE_VOID,
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

        auto decoratedAst = ast->clone();

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

        auto decoratedAst = ast->clone();

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

        auto decoratedAst = ast->clone();

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

        auto decoratedAst = ast->clone();

        Metadata expectedMetadata;

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, ConfigTests, Setup) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {},
            {
                new AstNode::Function::Oblock(
                    "foo",
                    {
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            PRIMITIVE_INT,
                            {}
                        )
                    },
                    {
                        "L1",
                        "L2",
                        "L3"
                    },
                    {},
                    {}
                )
            },
            new AstNode::Setup(
                {
                    new AstNode::Statement::Declaration(
                        "writer_1",
                        {},
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            std::string("host-1::file-f1.txt")
                        )
                    ),
                    new AstNode::Statement::Declaration(
                        "writer_2",
                        {RESERVED_VIRTUAL},
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            std::string("host-1::file-f2.txt")
                        )
                    ),
                    new AstNode::Statement::Declaration(
                        "signaler",
                        {RESERVED_VIRTUAL},
                        new AstNode::Specifier::Type(
                            PRIMITIVE_INT,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            int64_t(12)
                        )
                    ),
                    new AstNode::Statement::Expression(
                        new AstNode::Expression::Function(
                            "foo",
                            {
                                new AstNode::Expression::Access(
                                    "writer_1"
                                ),
                                new AstNode::Expression::Access(
                                    "writer_2"
                                ),
                                new AstNode::Expression::Access(
                                    "signaler"
                                )
                            }
                        )
                    )
                }
            )
        ));
    
        auto decoratedAst = ast->clone();

        Metadata expectedMetadata;

        expectedMetadata.deviceDescriptors = {
            {"writer_1", DeviceDescriptor{
                .device_name = "writer_1",
                .type = TYPE::LINE_WRITER,
                .controller = "host-1",
                .port_maps = {
                    {"file", "f1.txt"}
                },
                .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                .deviceKind = DeviceKind::INTERRUPT
            }},
            {"writer_2", DeviceDescriptor{
                .device_name = "writer_2",
                .type = TYPE::LINE_WRITER,
                .controller = "host-1",
                .port_maps = {
                    {"file", "f2.txt"}
                },
                .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                .isVtype = true,
                .deviceKind = DeviceKind::INTERRUPT
            }},
            {"signaler", DeviceDescriptor{
                .device_name = "signaler",
                .type = TYPE::int_t,
                .controller = "MASTER",
                .port_maps = {},
                .initialValue = int64_t(12),
                .isVtype = true
            }}
        };

        expectedMetadata.oblockDescriptors = {
            {"foo", OBlockDesc{
                .name = "foo",
                .binded_devices = {
                    DeviceDescriptor{
                        .device_name = "writer_1",
                        .type = TYPE::LINE_WRITER,
                        .controller = "host-1",
                        .port_maps = {
                            {"file", "f1.txt"}
                        },
                        .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                        .deviceKind = DeviceKind::INTERRUPT
                    },
                    DeviceDescriptor{
                        .device_name = "writer_2",
                        .type = TYPE::LINE_WRITER,
                        .controller = "host-1",
                        .port_maps = {
                            {"file", "f2.txt"}
                        },
                        .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                        .isVtype = true,
                        .deviceKind = DeviceKind::INTERRUPT
                    },
                    DeviceDescriptor{
                        .device_name = "signaler",
                        .type = TYPE::int_t,
                        .controller = "MASTER",
                        .port_maps = {},
                        .initialValue = int64_t(12),
                        .isVtype = true
                    }
                }
            }}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, ConfigTests, OblockConfig) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {},
            {
                new AstNode::Function::Oblock(
                    "foo",
                    {
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
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
                                new AstNode::Expression::Map(
                                    {
                                        {
                                            new AstNode::Expression::Literal(
                                                std::string("id")
                                            ),
                                            new AstNode::Expression::Literal(
                                                std::string("my trigger")
                                            )
                                        },
                                        {
                                            new AstNode::Expression::Literal(
                                                std::string("priority")
                                            ),
                                            new AstNode::Expression::Literal(
                                                int64_t(12)
                                            )
                                        },
                                        {
                                            new AstNode::Expression::Literal(
                                                std::string("rule")
                                            ),
                                            new AstNode::Expression::Access(
                                                "L3"
                                            ),
                                        }
                                    }
                                )
                            }
                        ),
                        new AstNode::Initializer::Oblock(
                            "constPollOn",
                            {
                                new AstNode::Expression::Map(
                                    {
                                        {
                                            new AstNode::Expression::Access(
                                                "L1"
                                            ),
                                            new AstNode::Expression::Literal(
                                                int64_t(10)
                                            )
                                        },
                                        {
                                            new AstNode::Expression::Access(
                                                "L2"
                                            ),
                                            new AstNode::Expression::Literal(
                                                double(6.28)
                                            )
                                        },
                                    }
                                )
                            }
                        ),
                        new AstNode::Initializer::Oblock(
                            "processPolicy",
                            {
                                new AstNode::Expression::Map(
                                    {
                                        {
                                            new AstNode::Expression::Access(
                                                "L1"
                                            ),
                                            new AstNode::Expression::Map(
                                                {
                                                    {
                                                        new AstNode::Expression::Literal(
                                                            std::string("read")
                                                        ),
                                                        new AstNode::Expression::Literal(
                                                            std::string("any")
                                                        )
                                                    }
                                                }
                                            )
                                        },
                                        {
                                            new AstNode::Expression::Access(
                                                "L3"
                                            ),
                                            new AstNode::Expression::Map(
                                                {
                                                    {
                                                        new AstNode::Expression::Literal(
                                                            std::string("yield")
                                                        ),
                                                        new AstNode::Expression::Literal(
                                                            bool(false)
                                                        )
                                                    }
                                                }
                                            )
                                        },
                                    }
                                )
                            }
                        ),
                        new AstNode::Initializer::Oblock(
                            "overwritePolicy",
                            {
                                new AstNode::Expression::Map(
                                    {
                                        {
                                            new AstNode::Expression::Access(
                                                "L2"
                                            ),
                                            new AstNode::Expression::Literal(
                                                std::string("clear")
                                            )
                                        }
                                    }
                                )
                            }
                        )
                    },
                    {}
                )
            },
            new AstNode::Setup(
                {
                    new AstNode::Statement::Declaration(
                        "writer_1",
                        {},
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            std::string("host-1::file-f1.txt")
                        )
                    ),
                    new AstNode::Statement::Declaration(
                        "writer_2",
                        {},
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            std::string("host-1::file-f2.txt")
                        )
                    ),
                    new AstNode::Statement::Declaration(
                        "writer_3",
                        {},
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            std::string("host-1::file-f3.txt")
                        )
                    ),
                    new AstNode::Statement::Expression(
                        new AstNode::Expression::Function(
                            "foo",
                            {
                                new AstNode::Expression::Access(
                                    "writer_1"
                                ),
                                new AstNode::Expression::Access(
                                    "writer_2"
                                ),
                                new AstNode::Expression::Access(
                                    "writer_3"
                                )
                            }
                        )
                    )
                }
            )
        ));
        
        auto decoratedAst = ast.get()->clone();

        Metadata expectedMetadata;

        expectedMetadata.deviceDescriptors = {
            {"writer_1", DeviceDescriptor{
                .device_name = "writer_1",
                .type = TYPE::LINE_WRITER,
                .controller = "host-1",
                .port_maps = {
                    {"file", "f1.txt"}
                },
                .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                .deviceKind = DeviceKind::INTERRUPT
            }},
            {"writer_2", DeviceDescriptor{
                .device_name = "writer_2",
                .type = TYPE::LINE_WRITER,
                .controller = "host-1",
                .port_maps = {
                    {"file", "f2.txt"}
                },
                .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                .deviceKind = DeviceKind::INTERRUPT
            }},
            {"writer_3", DeviceDescriptor{
                .device_name = "writer_3",
                .type = TYPE::LINE_WRITER,
                .controller = "host-1",
                .port_maps = {
                    {"file", "f3.txt"}
                },
                .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                .deviceKind = DeviceKind::INTERRUPT
            }}
        };

        expectedMetadata.oblockDescriptors = {
            {"foo", OBlockDesc{
                .name = "foo",
                .binded_devices = {
                    DeviceDescriptor{
                        .device_name = "writer_1",
                        .type = TYPE::LINE_WRITER,
                        .controller = "host-1",
                        .port_maps = {
                            {"file", "f1.txt"}
                        },
                        .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                        .readPolicy = READ_POLICY::ANY,
                        .polling_period = 10,
                        .isConst = true,
                        .deviceKind = DeviceKind::INTERRUPT
                    },
                    DeviceDescriptor{
                        .device_name = "writer_2",
                        .type = TYPE::LINE_WRITER,
                        .controller = "host-1",
                        .port_maps = {
                            {"file", "f2.txt"}
                        },
                        .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                        .overwritePolicy = OVERWRITE_POLICY::CLEAR,
                        .polling_period = 6,
                        .isConst = true,
                        .deviceKind = DeviceKind::INTERRUPT
                    },
                    DeviceDescriptor{
                        .device_name = "writer_3",
                        .type = TYPE::LINE_WRITER,
                        .controller = "host-1",
                        .port_maps = {
                            {"file", "f3.txt"}
                        },
                        .initialValue = createBlsType(TypeDef::LINE_WRITER()),
                        .isYield = false,
                        .deviceKind = DeviceKind::INTERRUPT
                    }
                },
                .hostController = "host-1",
                .triggers = {
                    TriggerData{{"writer_1", "writer_2"}},
                    TriggerData{{"writer_3"}, "my trigger", 12}
                }
            }}
        };

        TEST_ANALYZE(ast, decoratedAst, expectedMetadata);
    }

    GROUP_TEST_F(AnalyzerTest, ConfigTests, InvalidOblockOption) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Oblock(
            "foo",
            {
                new AstNode::Specifier::Type(
                    DEVTYPE_LINE_WRITER,
                    {}
                ),
                new AstNode::Specifier::Type(
                    DEVTYPE_LINE_WRITER,
                    {}
                ),
                new AstNode::Specifier::Type(
                    DEVTYPE_LINE_WRITER,
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
                    "badOption",
                    {}
                )
            },
            {}
        ));
    
        std::unique_ptr<AstNode> decoratedAst = nullptr;

        Metadata expectedMetadata;

        EXPECT_THROW(TEST_ANALYZE(ast, decoratedAst, expectedMetadata), SemanticError);
    }

    GROUP_TEST_F(AnalyzerTest, ConfigTests, InvalidOblockOptionArgument) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Oblock(
            "foo",
            {
                new AstNode::Specifier::Type(
                    DEVTYPE_LINE_WRITER,
                    {}
                ),
                new AstNode::Specifier::Type(
                    DEVTYPE_LINE_WRITER,
                    {}
                ),
                new AstNode::Specifier::Type(
                    DEVTYPE_LINE_WRITER,
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
                                new AstNode::Expression::Literal(
                                    int64_t(2)
                                )
                            }
                        ),
                        new AstNode::Expression::Access(
                            "L3"
                        ),
                    }
                ),
                new AstNode::Initializer::Oblock(
                    "badOption",
                    {}
                )
            },
            {}
        ));
    
        std::unique_ptr<AstNode> decoratedAst = nullptr;

        Metadata expectedMetadata;

        EXPECT_THROW(TEST_ANALYZE(ast, decoratedAst, expectedMetadata), SemanticError);
    }

}