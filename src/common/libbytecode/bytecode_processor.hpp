#pragma once
#include "include/Common.hpp"
#include "libtypes/bls_types.hpp"
#include <fstream>
#include <cstdint>

class BytecodeProcessor {
    public:
        void loadBytecode(const std::string& filename);
        void dispatch();

    private:
        void readHeader();
        void loadLiterals();

    protected:
        #define OPCODE_BEGIN(code) \
        virtual void code(
        #define ARGUMENT(arg, type) \
        type arg,
        #define OPCODE_END(...) \
        int = 0) = 0;
        #include "include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END

        std::ifstream bytecode;
        size_t instruction, instructionOffset;
        std::vector<BlsType> literalPool;
        std::vector<OBlockDesc> oblockDescs;
};