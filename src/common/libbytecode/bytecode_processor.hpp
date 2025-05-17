#pragma once
#include "include/Common.hpp"
#include "libbytecode/include/opcodes.hpp"
#include "libtypes/bls_types.hpp"
#include <fstream>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class BytecodeProcessor {
    public:
        enum class SIGNAL : uint8_t {
            SIGSTART,
            SIGSTOP,
            COUNT
        };

        void loadBytecode(const std::string& filename);
        void dispatch();
        std::vector<OBlockDesc> getOblockDescriptors() { return oblockDescs; }

    private:
        void readHeader();
        void loadLiterals();
        void loadInstructions();

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
        size_t instruction = 0;
        std::vector<OBlockDesc> oblockDescs;
        std::vector<BlsType> literalPool;
        std::vector<std::unique_ptr<INSTRUCTION>> instructions;
        SIGNAL signal = SIGNAL::SIGSTART;
};