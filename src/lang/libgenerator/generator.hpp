#pragma once
#include "include/Common.hpp"
#include "libbytecode/include/opcodes.hpp"
#include "libtypes/bls_types.hpp"
#include "visitor.hpp"
#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {

    class Generator : public Visitor {
        public:
            Generator(std::ostream& outputStream
                    , std::unordered_map<std::string, OBlockDesc> oblockDescriptors
                    , std::vector<BlsType> literalPool)
                    : outputStream(outputStream)
                    , oblockDescriptors(oblockDescriptors)
                    , literalPool(literalPool) {}

            #define AST_NODE_ABSTRACT(...)
            #define AST_NODE(Node) \
            std::any visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE
        
        private:
            #define OPCODE_BEGIN(code) \
            std::unique_ptr<INSTRUCTION::code> create##code(
            #define ARGUMENT(arg, type) \
            type arg,
            #define OPCODE_END(code, args...) \
            int = 0);
            #include "libbytecode/include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END

            std::ostream& outputStream;
            std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
            std::vector<BlsType> literalPool;
            std::unordered_map<std::string, INSTRUCTION::CALL> procedureMap;
            std::vector<std::unique_ptr<INSTRUCTION>> instructions;
    };

}