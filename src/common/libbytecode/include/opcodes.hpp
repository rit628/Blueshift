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
