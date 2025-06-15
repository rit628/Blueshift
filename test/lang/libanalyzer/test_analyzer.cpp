#include "ast.hpp"
#include "error_types.hpp"
#include "fixtures/analyzer_test.hpp"
#include "include/Common.hpp"
#include "include/reserved_tokens.hpp"
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

    /* TODO: Add tests checking proper argument adherence once new method system is established */
    
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
                            DEVTYPE_LINE_WRITER,
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
                        "writer_3",
                        {},
                        new AstNode::Specifier::Type(
                            DEVTYPE_LINE_WRITER,
                            {}
                        ),
                        new AstNode::Expression::Literal(
                            std::string("host-2::file-f3.txt")
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
    
        auto decoratedAst = ast->clone();

        Metadata expectedMetadata;

        expectedMetadata.deviceDescriptors = {
            {"writer_1", DeviceDescriptor{
                "writer_1",
                TYPE::LINE_WRITER,
                "host-1",
                {
                    {"file", "f1.txt"}
                }
            }},
            {"writer_2", DeviceDescriptor{
                "writer_2",
                TYPE::LINE_WRITER,
                "host-1",
                {
                    {"file", "f2.txt"}
                },
                true
            }},
            {"writer_3", DeviceDescriptor{
                "writer_3",
                TYPE::LINE_WRITER,
                "host-2",
                {
                    {"file", "f3.txt"}
                }
            }}
        };

        expectedMetadata.oblockDescriptors = {
            {"foo", OBlockDesc{
                "foo",
                {
                    DeviceDescriptor{
                        "writer_1",
                        TYPE::LINE_WRITER,
                        "host-1",
                        {
                            {"file", "f1.txt"}
                        }
                    },
                    DeviceDescriptor{
                        "writer_2",
                        TYPE::LINE_WRITER,
                        "host-1",
                        {
                            {"file", "f2.txt"}
                        },
                        true
                    },
                    DeviceDescriptor{
                        "writer_3",
                        TYPE::LINE_WRITER,
                        "host-2",
                        {
                            {"file", "f3.txt"}
                        }
                    }
                },
                0,
                false,
                false,
                {},
                true
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
                            "constPoll",
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
                            "dropRead",
                            {}
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
                            std::string("host-2::file-f3.txt")
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
                "writer_1",
                TYPE::LINE_WRITER,
                "host-1",
                {
                    {"file", "f1.txt"}
                }
            }},
            {"writer_2", DeviceDescriptor{
                "writer_2",
                TYPE::LINE_WRITER,
                "host-1",
                {
                    {"file", "f2.txt"}
                },
                false
            }},
            {"writer_3", DeviceDescriptor{
                "writer_3",
                TYPE::LINE_WRITER,
                "host-2",
                {
                    {"file", "f3.txt"}
                }
            }}
        };

        expectedMetadata.oblockDescriptors = {
            {"foo", OBlockDesc{
                "foo",
                {
                    DeviceDescriptor{
                        "writer_1",
                        TYPE::LINE_WRITER,
                        "host-1",
                        {
                            {"file", "f1.txt"}
                        },
                        false,
                        10
                    },
                    DeviceDescriptor{
                        "writer_2",
                        TYPE::LINE_WRITER,
                        "host-1",
                        {
                            {"file", "f2.txt"}
                        },
                        false,
                        6
                    },
                    DeviceDescriptor{
                        "writer_3",
                        TYPE::LINE_WRITER,
                        "host-2",
                        {
                            {"file", "f3.txt"}
                        }
                    }
                },
                0,
                true,
                false,
                {
                    TriggerData{{"writer_1", "writer_2"}},
                    TriggerData{{"writer_3"}, "my trigger", 12}
                },
                true
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