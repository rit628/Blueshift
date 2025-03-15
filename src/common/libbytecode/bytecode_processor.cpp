#include "bytecode_processor.hpp"
#include "include/opcodes.hpp"
#include <cstdint>
#include <cstring>

void BytecodeProcessor::loadBytecode(const std::string& filename) {
    bytecode.open(filename, std::ios::binary);
    if (!bytecode.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
}

void BytecodeProcessor::dispatch() {
    char buf[UINT16_MAX];
    while (bytecode.get(buf[0])) {
        OPCODE code = static_cast<OPCODE>(buf[0]);
        switch (code) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: {
            #define ARGUMENT(arg, type, bytes) \
                type arg; \
                bytecode.read(buf, bytes); \
                std::memcpy(&arg, buf, bytes);
            #define OPCODE_END(code, args...) \
                code(args); \
            break; \
            } 
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
            default:
                std::cerr << "INVALID OPCODE" << std::endl;
            break;
        }
    }
}