#include "bytecode_processor.hpp"
#include "include/Common.hpp"
#include "include/opcodes.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtypes/bls_types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <variant>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>


void BytecodeProcessor::loadBytecode(const std::string& filename) {
    bytecode.open(filename, std::ios::binary);
    if (!bytecode.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    readHeader();
    loadLiterals();
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
                throw std::runtime_error("INVALID OPCODE");
            break;
        }
        if (bytecode.tellg() != instruction) { // instruction ptr modified by opcode (JMP, CALL, etc.)
            bytecode.seekg(instruction + instructionOffset); // values are relative to bytecode start
        }
    }
}

void BytecodeProcessor::readHeader() {
    uint16_t descriptorCount;
    bytecode.read(reinterpret_cast<char*>(&descriptorCount), sizeof(descriptorCount));
    boost::archive::binary_iarchive ia(bytecode);
    for (uint16_t i = 0; i < descriptorCount; i++) {
        OBlockDesc temp;
        ia >> temp;
        oblockDescs.push_back(temp);
    }
}

void BytecodeProcessor::loadLiterals() {
    uint16_t poolSize;
    bytecode.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));

    auto deserializeHeapDescriptor = [this]() {
        std::vector<char> serialized;
        auto size = serialized.size();
        bytecode.read(reinterpret_cast<char*>(&size), sizeof(size));
        serialized.resize(size);
        bytecode.read(reinterpret_cast<char*>(serialized.data()), size);
        DynamicMessage dmsg;
        dmsg.Capture(serialized);
        return dmsg.toTree();
    };

    for (uint16_t i = 0; i < poolSize; i++) {
        TYPE literalType;
        bytecode.read(reinterpret_cast<char*>(&literalType), sizeof(literalType));
        BlsType literal;
        switch (literalType) {
            case TYPE::bool_t: {
                bool val;
                bytecode.read(reinterpret_cast<char*>(&val), sizeof(val));
                literal = val;
                break;
            }
            case TYPE::int_t: {
                int64_t val;
                bytecode.read(reinterpret_cast<char*>(&val), sizeof(val));
                literal = val;
                break;
            }
            case TYPE::float_t: {
                double val;
                bytecode.read(reinterpret_cast<char*>(&val), sizeof(val));
                literal = val;
                break;
            }
            case TYPE::string_t: {
                std::string val;
                auto size = val.size();
                bytecode.read(reinterpret_cast<char*>(&size), sizeof(size));
                val.resize(size);
                bytecode.read(reinterpret_cast<char*>(val.data()), size);
                literal = val;
                break;
            }
            case TYPE::list_t: {
                auto enclosingMap = deserializeHeapDescriptor();
                BlsType target = "__SERIALIZED_LIST__";
                literal = enclosingMap->access(target);
                break;
            }
            case TYPE::map_t: {
                literal = deserializeHeapDescriptor();
                break;
            }
            default: {
                throw std::runtime_error("invalid literal");
            }
        }
        literalPool.push_back(literal);
    }
    instructionOffset = bytecode.tellg();
}