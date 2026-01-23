#pragma once
#include "Serialization.hpp"
#include "opcodes.hpp"
#include "bls_types.hpp"
#include <fstream>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>

namespace BlsTrap {
    struct Impl;
}

class ExecutionUnit;

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
        std::vector<TaskDescriptor> getTaskDescriptors() { return taskDescs; }

        friend struct BlsTrap::Impl;

    private:
        void readHeader(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void loadLiterals(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void loadInstructions(std::istream& bytecode);

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
        std::vector<TaskDescriptor> taskDescs;
        std::vector<BlsType> literalPool;
        std::vector<std::unique_ptr<INSTRUCTION>> instructions;
        SIGNAL signal = SIGNAL::START;
        ExecutionUnit* ownerUnit = nullptr; // should be a member of VM but here for now to avoid circular header deps
};