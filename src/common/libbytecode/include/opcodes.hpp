#pragma once
#include <cstdint>

enum class OPCODE : uint8_t {
    #define OPCODE_BEGIN(code) \
    code,
    #define ARGUMENT(...)
    #define OPCODE_END(...)
    #include "OPCODES.LIST"
    #undef OPCODE_BEGIN
    #undef ARGUMENT
    #undef OPCODE_END
    COUNT
};

struct INSTRUCTION {
    #define OPCODE_BEGIN(code) \
    struct code;
    #define ARGUMENT(...)
    #define OPCODE_END(...)
    #include "OPCODES.LIST"
    #undef OPCODE_BEGIN
    #undef ARGUMENT
    #undef OPCODE_END
    OPCODE opcode;
};

#define OPCODE_BEGIN(code) \
struct INSTRUCTION::code : public INSTRUCTION {
#define ARGUMENT(arg, type) \
    type arg;
#define OPCODE_END(...) \
};
#include "OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END