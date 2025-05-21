#include "ast.hpp"
#include "fixtures/generator_test.hpp"
#include "include/Common.hpp"
#include "include/reserved_tokens.hpp"
#include "libbytecode/include/opcodes.hpp"
#include "libtypes/bls_types.hpp"
#include "test_macros.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {

    GROUP_TEST_F(GeneratorTest, StatementTests, Declaration) {
        auto ast = std::unique_ptr<AstNode>(new AstNode::Statement::Declaration(
            "x",
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
            GeneratorTest::createMKTYPE(0, static_cast<uint8_t>(TYPE::int_t))
        );

        TEST_GENERATE(ast, expectedInstructions);
    }

}