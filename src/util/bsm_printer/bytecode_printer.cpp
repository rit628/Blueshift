#include "bytecode_printer.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <cstdint>

template<typename... Args>
void BytecodePrinter::printArgs(Args... args) {
    ((std::cout << " " << args), ...);
}

#define OPCODE_BEGIN(code) \
void BytecodePrinter::code(
#define ARGUMENT(arg, type, ...) \
    type arg,
#define OPCODE_END(code, args...) \
    int) { \
    std::cout << #code; \
    printArgs(args);\
    std::cout << std::endl; \
    }
#include "libbytecode/include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END