#pragma once
#include "ast.hpp"
#include "Serialization.hpp"
#include "opcodes.hpp"
#include <concepts>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/range/combine.hpp>
#include "generator.hpp"
#include "bls_types.hpp"

namespace BlsLang {
    class GeneratorTest : public testing::Test {
        public:
            void INIT(std::unordered_map<std::string, TaskDescriptor>& taskDescriptors, std::unordered_map<BlsType, uint8_t>& literalPool) {
                generator = std::make_unique<Generator>(Generator(taskDescriptors, literalPool, functionSymbols));
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
                        #include "include/OPCODES.LIST"
                        #undef OPCODE_BEGIN
                        #undef ARGUMENT
                        #undef OPCODE_END
                        default:
                            ASSERT_NE(instruction->opcode, OPCODE::COUNT); // will always fail
                        break;
                    }
                }
            }
        
            template<typename... InstructionPtr> requires (std::convertible_to<InstructionPtr, std::unique_ptr<INSTRUCTION>> && ...)
            static std::vector<std::unique_ptr<INSTRUCTION>> makeInstructions(InstructionPtr&&... instructions) {
                std::vector<std::unique_ptr<INSTRUCTION>> container;
                ( container.push_back(std::forward<InstructionPtr>(instructions)), ... );
                return container;
            }

        private:
            bool INIT_FLAG = false;
            std::remove_reference_t<decltype(Generator::functionSymbols)> functionSymbols; // just to satisfy constructor
            std::unique_ptr<Generator> generator = nullptr;
    };
}