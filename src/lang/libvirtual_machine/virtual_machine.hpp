#pragma once
#include "bls_types.hpp"
#include "call_stack.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include <cstddef>
#include <vector>


namespace BlsLang {
    
    class VirtualMachine : public BytecodeProcessor {
        protected:
            #define OPCODE_BEGIN(code) \
            void code(
            #define ARGUMENT(arg, type, ...) \
            type arg,
            #define OPCODE_END(...) \
            int = 0) override;
            #include "libbytecode/include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
        
        private:
            CallStack<size_t> cs;
            size_t instruction;
    };

}