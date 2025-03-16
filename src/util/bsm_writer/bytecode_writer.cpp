#include "bytecode_writer.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <cstdint>
#include <sstream>
#include "libbytecode/include/opcodes.hpp"

void BytecodeWriter::loadMnemonicBytecode(const std::string& filename) {
    mnemonicBytecode.open(filename);
    if (!mnemonicBytecode.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
}

void BytecodeWriter::convertToBinary(std::ostream& stream) {
    std::stringstream ss;
    ss << mnemonicBytecode.rdbuf();
    std::string buf;
    while (ss >> buf) {
        if (false) {} // hack to force short circuiting and invalid input checking
        #define OPCODE_BEGIN(code) \
        else if (buf == #code) { \
            OPCODE c = OPCODE::code; \
            stream.write(reinterpret_cast<const char *>(&c), 1);
        #define ARGUMENT(arg, type) \
            type arg; \
            ss >> arg; \
            stream.write(reinterpret_cast<const char *>(&arg), sizeof(type));
        #define OPCODE_END(...) \
        }
        #include "libbytecode/include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END
        else {
            std::cerr << "INVALID OPCODE: " << buf << std::endl;
        }
    }
}