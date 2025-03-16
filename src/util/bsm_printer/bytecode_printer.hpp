#pragma once
#include "libbytecode/bytecode_processor.hpp"
#include <cstdint>

class BytecodePrinter : public BytecodeProcessor {
    public:
        void setOutputStream(std::ostream& stream = std::cout);

    protected:
        #define OPCODE_BEGIN(code) \
        void code(
        #define ARGUMENT(arg, type) \
        type arg,
        #define OPCODE_END(...) \
        int = 0) override;
        #include "libbytecode/include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END
    
    private:
        template<typename... Args>
        void printArgs(Args... args);

        std::ostream* outputStream;
};