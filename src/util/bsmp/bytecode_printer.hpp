#pragma once

#include "bytecode_processor.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>


class BytecodePrinter : public BytecodeProcessor {
    public:
        void setOutputStream(std::ostream& stream = std::cout);
        void printHeader();
        void printLiteralPool();

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
        template<typename... Args>
        void printArgs(Args... args);
        void printCALL(uint16_t address, uint8_t argc);
        void printEMIT(uint8_t signal);
        void printPUSH(uint8_t index);
        void printMKTYPE(uint8_t index, uint8_t type);
        void printSTORE(uint8_t index);
        void printLOAD(uint8_t index);
        void printMTRAP(uint16_t callnum);
        void printTRAP(uint16_t callnum, uint8_t argc);
        /* C style overloads of specialized functions to get around xmacro error in if constexpr for uncompiled branches */
        void printCALL(...) { throw std::runtime_error("CALL PRETTY PRINT OUT OF DATE"); }
        void printEMIT(...) { throw std::runtime_error("EMIT PRETTY PRINT OUT OF DATE"); }
        void printPUSH(...) { throw std::runtime_error("PUSH PRETTY PRINT OUT OF DATE"); }
        void printMKTYPE(...) { throw std::runtime_error("MKTYPE PRETTY PRINT OUT OF DATE"); }
        void printSTORE(...) { throw std::runtime_error("STORE PRETTY PRINT OUT OF DATE"); }
        void printLOAD(...) { throw std::runtime_error("LOAD PRETTY PRINT OUT OF DATE"); }
        void printMTRAP(...) { throw std::runtime_error("MTRAP PRETTY PRINT OUT OF DATE"); }
        void printTRAP(...) { throw std::runtime_error("TRAP PRETTY PRINT OUT OF DATE"); }

        std::ostream* outputStream;
        std::vector<std::string>* currentFunctionSymbols;
};