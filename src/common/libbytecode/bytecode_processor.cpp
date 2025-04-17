#include "bytecode_processor.hpp"
#include "include/opcodes.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>

void BytecodeProcessor::loadBytecode(const std::string& filename) {
    bytecode.open(filename, std::ios::binary);
    if (!bytecode.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
}

void BytecodeProcessor::dispatch() {
    OPCODE code;
    while (bytecode.read(reinterpret_cast<char*>(&code), sizeof(code))) {
        switch (code) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: {
            #define ARGUMENT(arg, type) \
                type arg; \
                bytecode.read(reinterpret_cast<char*>(&arg), sizeof(type));
            #define OPCODE_END(code, args...) \
                instruction = bytecode.tellg(); \
                code(args); \
            break; \
            } 
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
            default:
                instruction = bytecode.tellg();
                std::cerr << "INVALID OPCODE" << std::endl;
            break;
        }
        if (bytecode.tellg() != instruction) { // instruction ptr modified by opcode (JMP, CALL, etc.)
            bytecode.seekg(instruction);
        }
    }
}