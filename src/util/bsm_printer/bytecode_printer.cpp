#include "bytecode_printer.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <cstdint>

void BytecodePrinter::setOutputStream(std::ostream& stream) {
    outputStream = &stream;
}

template<typename... Args>
void BytecodePrinter::printArgs(Args... args) {
    ((*outputStream << " " << args), ...);
}

#define OPCODE_BEGIN(code) \
void BytecodePrinter::code(
#define ARGUMENT(arg, type) \
    type arg,
#define OPCODE_END(code, args...) \
    int) { \
    *outputStream << #code; \
    printArgs(args);\
    *outputStream << std::endl; \
    }
#include "libbytecode/include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END