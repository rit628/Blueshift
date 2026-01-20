#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "libbytecode/opcodes.hpp"
#include <concepts>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/range/combine.hpp>
#include "generator.hpp"
#include "libtype/bls_types.hpp"

namespace BlsLang {
    class GeneratorTest : public testing::Test {
        public:
            void INIT(std::unordered_map<std::string, TaskDescriptor>& taskDescriptors, std::unordered_map<BlsType, uint8_t>& literalPool) {
                generator = std::make_unique<Generator>(Generator(taskDescriptors, literalPool));
                INIT_FLAG = true;
            }

            void TEST_GENERATE(std::unique_ptr<AstNode>& ast, std::vector<std::unique_ptr<INSTRUCTION>>& expectedInstructions) {
                ASSERT_TRUE(INIT_FLAG);
                ast->accept(*generator);
                ASSERT_EQ(generator->instructions.size(), expectedInstructions.size());
                for (auto&& [instruction, expectedInstruction] : boost::combine(generator->instructions, expectedInstructions)) {
                    ASSERT_EQ(instruction->opcode, expectedInstruction->opcode);
                    switch (instruction->opcode) {
                        #define OPCODE_BEGIN(code) \
                        case OPCODE::code: { \
                            auto& resolvedInstruction [[ maybe_unused ]] = static_cast<INSTRUCTION::code&>(*instruction); \
                            auto& resolvedExpected [[ maybe_unused ]] = static_cast<INSTRUCTION::code&>(*expectedInstruction);
                        #define ARGUMENT(arg, ...) \
                            EXPECT_EQ(resolvedInstruction.arg, resolvedExpected.arg);
                        #define OPCODE_END(...) \
                            break; \
                        }
                        #include "libbytecode/include/OPCODES.LIST"
                        #undef OPCODE_BEGIN
                        #undef ARGUMENT
                        #undef OPCODE_END
                        default:
                            ASSERT_NE(instruction->opcode, OPCODE::COUNT); // will always fail
                        break;
                    }
                }
            }

            #define OPCODE_BEGIN(code) \
            static std::unique_ptr<INSTRUCTION::code> create##code(
            #define ARGUMENT(arg, type) \
            type arg,
            #define OPCODE_END(code, args...) \
            int = 0) { \
                return Generator::create##code(args); \
            }
            #include "libbytecode/include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
        
            template<typename... InstructionPtr> requires (std::convertible_to<InstructionPtr, std::unique_ptr<INSTRUCTION>> && ...)
            static std::vector<std::unique_ptr<INSTRUCTION>> makeInstructions(InstructionPtr&&... instructions) {
                std::vector<std::unique_ptr<INSTRUCTION>> container;
                ( container.push_back(std::forward<InstructionPtr>(instructions)), ... );
                return container;
            }

        private:
            bool INIT_FLAG = false;
            std::unique_ptr<Generator> generator = nullptr;
    };
}