#pragma once
#include <cstdint>
#include <memory>

enum class OPCODE : uint8_t {
    #define OPCODE_BEGIN(code) \
    code,
    #define ARGUMENT(...)
    #define OPCODE_END(...)
    #include "include/OPCODES.LIST"
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
    #include "include/OPCODES.LIST"
    #undef OPCODE_BEGIN
    #undef ARGUMENT
    #undef OPCODE_END
    OPCODE opcode = OPCODE::COUNT;
};

#define OPCODE_BEGIN(code) \
struct INSTRUCTION::code : public INSTRUCTION {
#define ARGUMENT(arg, type) \
    type arg = type();
#define OPCODE_END(...) \
};
#include "include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END

#define OPCODE_BEGIN(code) \
inline std::unique_ptr<INSTRUCTION::code> create##code(
#define ARGUMENT(arg, type) \
type arg = type(),
#define OPCODE_END(code, args...) \
int = 0) { \
    return std::make_unique<INSTRUCTION::code>(INSTRUCTION::code{{OPCODE::code}, args}); \
}
#include "include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END