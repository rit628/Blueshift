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
    char buf[sizeof(uint16_t) + 1];
    while (bytecode.get(buf[0])) {
        OPCODE code = static_cast<OPCODE>(buf[0]);
        uint16_t currAddress;
        switch (code) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: {
            #define ARGUMENT(arg, type) \
                type arg; \
                bytecode.read(buf, sizeof(type)); \
                std::memcpy(&arg, buf, sizeof(type)); \
                instruction += sizeof(type);
            #define OPCODE_END(code, args...) \
                currAddress = ++instruction; \
                code(args); \
            break; \
            } 
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
            default:
                currAddress = ++instruction;
                std::cerr << "INVALID OPCODE" << std::endl;
            break;
        }
        if (currAddress != instruction) { // instruction ptr modified by opcode (JMP, CALL, etc.)
            bytecode.seekg(instruction);
        }
    }
}