#include "ast.hpp"
#include "fixtures/generator_test.hpp"
#include "include/Common.hpp"
#include "include/reserved_tokens.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "libbytecode/opcodes.hpp"
#include "libtrap/include/traps.hpp"
#include "libtype/bls_types.hpp"
#include "test_macros.hpp"
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(GeneratorTest, ExpressionTests, VariableAccess) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "x",
            uint8_t(0)
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool;
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, ListAccess) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "x",
            new AstNode::Expression::Access(
                "y",
                uint8_t(1)
            ),
            uint8_t(0)
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool;
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0),
            createLOAD(1),
            createALOAD()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, MapAccess) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Access(
            "x",
            "member",
            uint8_t(0)
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"member", 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0),
            createPUSH(0),
            createALOAD()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryArithmetic) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "+",
            new AstNode::Expression::Literal(
                int64_t(20)
            ),
            new AstNode::Expression::Literal(
                int64_t(30)
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {20, 0},
            {30, 1}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createPUSH(1),
            createADD()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryCompoundArithmetic) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "+",
            new AstNode::Expression::Literal(
                int64_t(20)
            ),
            new AstNode::Expression::Group(
                new AstNode::Expression::Binary(
                    "-",
                    new AstNode::Expression::Literal(
                        int64_t(30)
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(10)
                    )
                )
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {20, 0},
            {30, 1},
            {10, 2}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createPUSH(1),
            createPUSH(2),
            createSUB(),
            createADD()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "=",
            new AstNode::Expression::Access(
                "x",
                uint8_t(0)
            ),
            new AstNode::Expression::Literal(
                int64_t(30)
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {30, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createSTORE(0),
            createLOAD(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryCompoundAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "*=",
            new AstNode::Expression::Access(
                "x",
                uint8_t(0)
            ),
            new AstNode::Expression::Literal(
                int64_t(30)
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {30, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0),
            createPUSH(0),
            createMUL(),
            createSTORE(0),
            createLOAD(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryDoubleAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "=",
            new AstNode::Expression::Access(
                "x",
                uint8_t(0)
            ),
            new AstNode::Expression::Binary(
                "=",
                new AstNode::Expression::Access(
                    "y",
                    uint8_t(1)
                ),
                new AstNode::Expression::Literal(
                    int64_t(30)
                )
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {30, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0), // push 30
            createSTORE(1), // assign to y
            createLOAD(1), // push value of y
            createSTORE(0), // assign to x
            createLOAD(0) // push value of x
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryDoubleCompoundAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "+=",
            new AstNode::Expression::Access(
                "x",
                uint8_t(0)
            ),
            new AstNode::Expression::Binary(
                "+=",
                new AstNode::Expression::Access(
                    "y",
                    uint8_t(1)
                ),
                new AstNode::Expression::Literal(
                    int64_t(30)
                )
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {30, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0), // load x as read target
            createLOAD(1), // load y as read target
            createPUSH(0), // push 30
            createADD(), // y = y + 30
            createSTORE(1), // assign to y
            createLOAD(1), // push value of y
            createADD(), // x = x + y
            createSTORE(0), // assign to x
            createLOAD(0) // push value of x
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, BinaryDoubleCompoundArrayAssignment) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Binary(
            "+=",
            new AstNode::Expression::Access(
                "x",
                new AstNode::Expression::Literal(
                    int64_t(0)
                ),
                uint8_t(0)
            ),
            new AstNode::Expression::Binary(
                "+=",
                new AstNode::Expression::Access(
                    "y",
                    new AstNode::Expression::Literal(
                        int64_t(0)
                    ),
                    uint8_t(1)
                ),
                new AstNode::Expression::Literal(
                    int64_t(30)
                )
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {0, 0},
            {30, 1},
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0), // load x[0] as write target
            createPUSH(0),
            
            createLOAD(0), // load x[0] as read target
            createPUSH(0),
            createALOAD(),

            createLOAD(1), // load y[0] as write target
            createPUSH(0),

            createLOAD(1), // load y[0] as read target
            createPUSH(0),
            createALOAD(),

            createPUSH(1), // add y[0] to 30 and store result in y[0]
            createADD(),
            createASTORE(),

            createLOAD(1), // load y[0] as op result
            createPUSH(0),
            createALOAD(),

            createADD(),
            createASTORE(), // add x[0] to y[0] and store result in x[0]

            createLOAD(0), // load x[0] as op result
            createPUSH(0),
            createALOAD()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, UnaryNot) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            "!",
            new AstNode::Expression::Literal(
                false
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {false, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createNOT()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, UnaryDoubleNot) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            "!",
            new AstNode::Expression::Unary(
                "!",
                new AstNode::Expression::Literal(
                    false
                )
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {false, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createNOT(),
            createNOT()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, UnaryPreIncrement) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            "++",
            new AstNode::Expression::Access(
                "x",
                uint8_t(0)
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {1, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0),
            createPUSH(0),
            createADD(),
            createSTORE(0),
            createLOAD(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, ExpressionTests, UnaryPostIncrement) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Unary(
            "++",
            new AstNode::Expression::Access(
                "x",
                uint8_t(0)
            ),
            AstNode::Expression::Unary::OPERATOR_POSITION::POSTFIX
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {1, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0),
            createLOAD(0),
            createPUSH(0),
            createADD(),
            createSTORE(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, String) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Literal(
            std::string("string")
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"string", 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, FullList) {
        auto listLiteral = std::make_shared<VectorDescriptor>(std::initializer_list<BlsType>({2, 50, 999}));

        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::List(
           {
                new AstNode::Expression::Literal(
                    int64_t(2)
                ),
                new AstNode::Expression::Literal(
                    int64_t(50)
                ),
                new AstNode::Expression::Literal(
                    int64_t(999)
                )
           },
           listLiteral
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {2, 0},
            {50, 1},
            {999, 2},
            {listLiteral, 3}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(3)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, PartialList) {
        auto listLiteral = std::make_shared<VectorDescriptor>(std::initializer_list<BlsType>({std::monostate(), 50, 999}));

        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::List(
           {
                new AstNode::Expression::Access(
                    "x",
                    uint8_t(0)
                ),
                new AstNode::Expression::Literal(
                    int64_t(50)
                ),
                new AstNode::Expression::Literal(
                    int64_t(999)
                )
           },
           listLiteral
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {0, 0},
            {50, 1},
            {999, 2},
            {listLiteral, 3}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(3), // push list
            createPUSH(0), // push index (0)
            createLOAD(0), // push value of x (index 0)
            createASTORE(),
            createPUSH(3)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, FullMap) {
        auto mapLiteral = std::make_shared<MapDescriptor>(std::initializer_list<std::pair<std::string, BlsType>>{
            {"k1", 12},
            {"k2", 14}
        });

        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Map(
            {
                {
                    new AstNode::Expression::Literal(
                        std::string("k1")
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(12)
                    )
                },
                {
                    new AstNode::Expression::Literal(
                        std::string("k2")
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(14)
                    )
                }
            },
            mapLiteral
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"k1", 0},
            {12, 1},
            {"k2", 2},
            {14, 3},
            {mapLiteral, 4}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(4)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, PartialValueMap) {
        auto mapLiteral = std::make_shared<MapDescriptor>(std::initializer_list<std::pair<std::string, BlsType>>{
            {"k1", 12},
            {"k2", std::monostate()}
        });

        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Map(
            {
                {
                    new AstNode::Expression::Literal(
                        std::string("k1")
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(12)
                    )
                },
                {
                    new AstNode::Expression::Literal(
                        std::string("k2")
                    ),
                    new AstNode::Expression::Access(
                        "x",
                        uint8_t(0)
                    )
                }
            },
            mapLiteral
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"k1", 0},
            {12, 1},
            {"k2", 2},
            {mapLiteral, 3}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(3), // push map
            createPUSH(2), // push key "k2" (index 2)
            createLOAD(0), // push value of x (index 0)
            createEMPLACE(),
            createPUSH(3)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, PartialKeyMap) {
        auto mapLiteral = std::make_shared<MapDescriptor>(std::initializer_list<std::pair<std::string, BlsType>>{
            {"k1", 12}
        });

        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Map(
            {
                {
                    new AstNode::Expression::Literal(
                        std::string("k1")
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(12)
                    )
                },
                {
                    new AstNode::Expression::Access(
                        "x",
                        uint8_t(0)
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(14)
                    )
                }
            },
            mapLiteral
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"k1", 0},
            {12, 1},
            {14, 2},
            {mapLiteral, 3}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(3), // push map
            createLOAD(0), // push value of key x (index 0)
            createPUSH(2), // push value 14 (index 2)
            createEMPLACE(),
            createPUSH(3)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, LiteralTests, PartialKeyValueMap) {
        auto mapLiteral = std::make_shared<MapDescriptor>(std::initializer_list<std::pair<std::string, BlsType>>{
            {"k1", 12}
        });

        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Map(
            {
                {
                    new AstNode::Expression::Literal(
                        std::string("k1")
                    ),
                    new AstNode::Expression::Literal(
                        int64_t(12)
                    )
                },
                {
                    new AstNode::Expression::Access(
                        "x",
                        uint8_t(0)
                    ),
                    new AstNode::Expression::Access(
                        "y",
                        uint8_t(1)
                    )
                }
            },
            mapLiteral
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"k1", 0},
            {12, 1},
            {mapLiteral, 2}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(2), // push map
            createLOAD(0), // push value of key x (index 0)
            createLOAD(1), // push value of y (index 1)
            createEMPLACE(),
            createPUSH(2)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, ValuelessDeclaration) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            {},
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {}
            ),
            std::nullopt,
            0
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool;
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t))
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, ValuedDeclaration) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
            {},
            new AstNode::Specifier::Type(
                PRIMITIVE_INT,
                {}
            ),
            new AstNode::Expression::Literal(
                int64_t(2)
            ),
            0
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {int64_t(2), 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createPUSH(0),
            createSTORE(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, SingleIf) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            },
            {},
            {}
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {true, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createBRANCH(4),
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(4)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, IfWithElseIf) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            },
            {
                new AstNode::Statement::If(
                    new AstNode::Expression::Literal(
                        false
                    ),
                    {
                        new AstNode::Statement::Declaration(
                            "y",
                            {},
                            new AstNode::Specifier::Type(
                                PRIMITIVE_FLOAT,
                                {}
                            ),
                            std::nullopt,
                            1
                        )
                    },
                    {},
                    {}
                )
            },
            {}
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {true, 0},
            {false, 1}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createBRANCH(4),
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(8),
            createPUSH(1),
            createBRANCH(8),
            createMKTYPE(1, static_cast<uint8_t>(TYPE::float_t)),
            createJMP(8)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, IfWithElseIfAndElse) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::If(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            },
            {
                new AstNode::Statement::If(
                    new AstNode::Expression::Literal(
                        false
                    ),
                    {
                        new AstNode::Statement::Declaration(
                            "y",
                            {},
                            new AstNode::Specifier::Type(
                                PRIMITIVE_FLOAT,
                                {}
                            ),
                            std::nullopt,
                            1
                        )
                    },
                    {},
                    {}
                )
            },
            {
                new AstNode::Statement::Declaration(
                    "z",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_STRING,
                        {}
                    ),
                    std::nullopt,
                    2
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {true, 0},
            {false, 1}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createBRANCH(4),
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(9),
            createPUSH(1),
            createBRANCH(8),
            createMKTYPE(1, static_cast<uint8_t>(TYPE::float_t)),
            createJMP(9),
            createMKTYPE(2, static_cast<uint8_t>(TYPE::string_t))
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, WhileTrue) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {true, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createBRANCH(4),
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, WhileTrueWithBreak) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Break(),
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {true, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createBRANCH(5),
            createJMP(5),
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, WhileTrueWithContinue) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::While(
            new AstNode::Expression::Literal(
                true
            ),
            {
                new AstNode::Statement::Continue(),
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {true, 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createBRANCH(5),
            createJMP(0),
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, StatementTests, EmptyFor) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::For(
            std::nullopt,
            std::nullopt,
            std::nullopt,
            {
                new AstNode::Statement::Declaration(
                    "x",
                    {},
                    new AstNode::Specifier::Type(
                        PRIMITIVE_INT,
                        {}
                    ),
                    std::nullopt,
                    0
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool;
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t)),
            createJMP(0)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, FunctionTests, RecursiveProcedureCall) {
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
                        "f",
                        {}
                    )
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {std::monostate(), 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createCALL(0, 0),
            createPUSH(0),
            createRETURN()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, FunctionTests, RecursiveProcedureCallWithArgs) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Function::Procedure(
            "f",
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
            {
                new AstNode::Statement::Expression(
                    new AstNode::Expression::Function(
                        "f",
                        {
                            new AstNode::Expression::Literal(
                                int64_t(1)
                            ),
                            new AstNode::Expression::Literal(
                                std::string("arg2")
                            )
                        }
                    )
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {1, 0},
            {"arg2", 1},
            {std::monostate(), 2}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createPUSH(1),
            createCALL(0, 2),
            createPUSH(2),
            createRETURN()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, FunctionTests, SeperateProcedureCall) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "f",
                    new AstNode::Specifier::Type(
                        "void",
                        {}
                    ),
                    {},
                    {},
                    {}
                ),
                new AstNode::Function::Procedure(
                    "g",
                    new AstNode::Specifier::Type(
                        "void",
                        {}
                    ),
                    {},
                    {},
                    {}
                ),
                new AstNode::Function::Procedure(
                    "h",
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
                )
            },
            {},
            new AstNode::Setup(
                {}
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {std::monostate(), 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createRETURN(), // end of f()
            createPUSH(0),
            createRETURN(), // end of g()
            createCALL(2, 0),
            createPUSH(0),
            createRETURN() // end of h()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, FunctionTests, OblockCallingProcedure) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Source(
            {
                new AstNode::Function::Procedure(
                    "f",
                    new AstNode::Specifier::Type(
                        "void",
                        {}
                    ),
                    {},
                    {},
                    {}
                ),
                new AstNode::Function::Procedure(
                    "g",
                    new AstNode::Specifier::Type(
                        "void",
                        {}
                    ),
                    {},
                    {},
                    {}
                )
            },
            {
                new AstNode::Function::Oblock(
                    "h",
                    {
                        new AstNode::Specifier::Type(
                            "LIGHT",
                            {}
                        )
                    },
                    {
                        "l1"
                    },
                    {},
                    {
                        new AstNode::Statement::Expression(
                            new AstNode::Expression::Function(
                                "g",
                                {}
                            )
                        )
                    }
                )
            },
            new AstNode::Setup(
                {}
            )
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors = {
            {"h", OBlockDesc("h")}
        };
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {std::monostate(), 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createRETURN(), // end of f()
            createPUSH(0),
            createRETURN(), // end of g()
            createCALL(2, 0),
            createEMIT(static_cast<uint8_t>(BytecodeProcessor::SIGNAL::STOP)) // end of h()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, FunctionTests, VariadicTrapCall) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Function(
            "print",
            {
                new AstNode::Expression::Literal(
                    std::string("arg1")
                ),
                new AstNode::Expression::Literal(
                    int64_t(100)
                ),
                new AstNode::Expression::Access(
                    "x",
                    uint8_t(0)
                )
            }
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"arg1", 0},
            {100, 1}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createPUSH(0),
            createPUSH(1),
            createLOAD(0),
            createTRAP(static_cast<uint16_t>(BlsTrap::CALLNUM::print), 3)
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

    GROUP_TEST_F(GeneratorTest, FunctionTests, MethodCall) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Expression::Method(
            "a",
            "emplace",
            {
                new AstNode::Expression::Literal(
                    std::string("key")
                ),
                new AstNode::Expression::Access(
                    "x",
                    uint8_t(1)
                )
            },
            uint8_t(0)
        ));

        std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
        std::unordered_map<BlsType, uint8_t> literalPool = {
            {"key", 0}
        };
        
        INIT(oblockDescriptors, literalPool);

        std::vector<std::unique_ptr<INSTRUCTION>> expectedInstructions = makeInstructions(
            createLOAD(0),
            createPUSH(0),
            createLOAD(1),
            createEMPLACE()
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

}