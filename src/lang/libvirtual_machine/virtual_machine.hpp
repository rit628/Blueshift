#pragma once
#include "traps.hpp"
#include "bls_types.hpp"
#include "call_stack.hpp"
#include "bytecode_processor.hpp"
#include <cstddef>
#include <vector>

class ExecutionUnit;

namespace BlsLang {
    
    class VirtualMachine : public BytecodeProcessor {
        public:
            void setParentExecutionUnit(ExecutionUnit* ownerUnit);
            void setTaskOffset(size_t taskOffset);
            std::vector<BlsType> transform(std::vector<BlsType> deviceStates);
            std::vector<bool>& getModifiedStates();

            friend struct BlsTrap::Impl;

        protected:
            #define OPCODE_BEGIN(code) \
            void code(
            #define ARGUMENT(arg, type) \
            type arg,
            #define OPCODE_END(...) \
            int = 0) override;
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
        
        private:
            std::vector<bool> modifiedStates;
            size_t taskOffset = 0;
            CallStack<size_t> cs;
    };

}