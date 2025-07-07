#pragma once
#include "libtype/bls_types.hpp"
#include "call_stack.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include <cstddef>
#include <vector>


namespace BlsLang {
    
    class VirtualMachine : public BytecodeProcessor {
        public:
            void setOblockOffset(size_t oblockOffset);
            std::vector<BlsType> transform(std::vector<BlsType> deviceStates);

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
            size_t oblockOffset = 0;
            CallStack<size_t> cs;
    };

}