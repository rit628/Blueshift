#pragma once
#include "include/Common.hpp"
#include "libbytecode/opcodes.hpp"
#include "libtype/bls_types.hpp"
#include <fstream>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>

class BytecodeProcessor {
    public:
        enum class SIGNAL : uint8_t {
            START,
            STOP,
            COUNT
        };

        void loadBytecode(std::istream& bytecode);
        void loadBytecode(const std::string& filename);
        void loadBytecode(const std::vector<char>& bytecode);
        void dispatch();
        std::vector<OBlockDesc> getOblockDescriptors() { return oblockDescs; }

    private:
        void readHeader(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void loadLiterals(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void loadInstructions(std::istream& bytecode, boost::archive::binary_iarchive& ia);

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

        size_t instruction = 0;
        std::vector<OBlockDesc> oblockDescs;
        std::vector<BlsType> literalPool;
        std::vector<std::unique_ptr<INSTRUCTION>> instructions;
        SIGNAL signal = SIGNAL::START;
};