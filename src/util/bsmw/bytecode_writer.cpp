#include "bytecode_writer.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtype/bls_types.hpp"
#include "libbytecode/opcodes.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace boost::json;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

static std::stringstream& operator>>(std::stringstream& ss, uint8_t& num) {
    uint16_t temp;
    ss >> temp;
    num = temp;
    return ss;
}

void BytecodeWriter::loadMnemonicBytecode(const std::string& filename) {
    std::fstream input;
    input.open(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    mnemonicBytecode << input.rdbuf();
}

void BytecodeWriter::writeHeader(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf, oblockDescJSON;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != "LITERALS_BEGIN")) {
        oblockDescJSON += buf;
    }
    array descArray = parse(oblockDescJSON).get_array();
    uint16_t descSize = descArray.size();
    stream.write(reinterpret_cast<const char *>(&descSize), sizeof(descSize));
    for (auto&& desc : descArray) {
        oa << value_to<OBlockDesc>(desc);
    }
}

void BytecodeWriter::writeLiteralPool(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf, poolJSON;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != "BYTECODE_BEGIN")) {
        poolJSON += buf;
    }
    array pool = parse(poolJSON).get_array();

    uint16_t poolSize = pool.size();
    stream.write(reinterpret_cast<const char *>(&poolSize), sizeof(poolSize));
    
    for (auto&& literal : pool) {
        oa << value_to<BlsType>(literal);
    }
}

void BytecodeWriter::writeBody(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf;
    while (mnemonicBytecode >> buf) {
        if (false) { } // hack to force short circuiting and invalid input checking
        #define OPCODE_BEGIN(code) \
        else if (buf == #code) { \
            OPCODE c = OPCODE::code; \
            stream.write(reinterpret_cast<const char *>(&c), sizeof(c));
        #define ARGUMENT(arg, type) \
            type arg; \
            mnemonicBytecode >> arg; \
            stream.write(reinterpret_cast<const char *>(&arg), sizeof(type));
        #define OPCODE_END(...) \
        }
        #include "libbytecode/include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END
        else if (buf.ends_with(':')) { // oblock/procedure labels
            continue;
        }
        else {
            throw std::runtime_error("INVALID OPCODE: " + buf);
        }
    }
}

void BytecodeWriter::convertToBinary(std::ostream& stream) {
    if (stream.bad()) {
        throw std::runtime_error("Bad output stream provided.");
    }
    boost::archive::binary_oarchive oa(stream, boost::archive::archive_flags::no_header);
    writeHeader(stream, oa);
    writeLiteralPool(stream, oa);
    writeBody(stream, oa);
}