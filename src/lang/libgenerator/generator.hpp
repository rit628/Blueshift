#pragma once
#include "Serialization.hpp"
#include "opcodes.hpp"
#include "bls_types.hpp"
#include "visitor.hpp"
#include <cstdint>
#include <ostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {

    class Generator : public Visitor {
        public:
            friend class GeneratorTest;
            Generator(std::unordered_map<std::string, TaskDescriptor>& taskDescriptors
                    , std::unordered_map<BlsType, uint8_t>& literalPool)
                    : taskDescriptors(taskDescriptors)
                    , literalPool(literalPool) {}

            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE

            void writeBytecode(std::ostream& outputStream);
            void writeBytecode(std::vector<char>& outputVector);
        
        private:
            enum class ACCESS_CONTEXT : uint8_t {
                  READ
                , WRITE
            };

            enum class FUNCTION_CONTEXT : uint8_t {
                  PROCEDURE
                , TASK
            };

            #define OPCODE_BEGIN(code) \
            static std::unique_ptr<INSTRUCTION::code> create##code(
            #define ARGUMENT(arg, type) \
            type arg,
            #define OPCODE_END(code, args...) \
            int = 0);
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END

            std::unordered_map<std::string, TaskDescriptor>& taskDescriptors;
            std::unordered_map<BlsType, uint8_t>& literalPool;
            std::unordered_map<std::string, uint16_t> procedureAddresses;
            std::vector<std::unique_ptr<INSTRUCTION>> instructions;
            std::stack<std::stack<uint16_t>> continueIndices, breakIndices; // needed for break and continue generation
            ACCESS_CONTEXT accessContext = ACCESS_CONTEXT::READ; // needed for assignment generation
            FUNCTION_CONTEXT functionContext = FUNCTION_CONTEXT::PROCEDURE;
    };

}