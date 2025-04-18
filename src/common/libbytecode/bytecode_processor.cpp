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
    loadInstructions();
}

void BytecodeProcessor::dispatch() {
    while (instruction != instructions.size()) {
        auto& instructionStruct = instructions[instruction];
        instruction++;
        switch (instructionStruct->opcode) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: { \
                auto& resolvedInstruction = reinterpret_cast<INSTRUCTION::code&>(*instructionStruct);
            #define ARGUMENT(arg, type) \
                type& arg = resolvedInstruction.arg;
            #define OPCODE_END(code, args...) \
                code(args); \
                break; \
            } 
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
            default:
            break;
        }
        switch (signal) {
            case SIGNAL::SIGSTART:
            break;
            case SIGNAL::SIGSTOP:
                goto exit;
            break;
            default:
            break;
        }
    }
    exit: ;
}

void BytecodeProcessor::readHeader() {
    uint16_t descriptorCount;
    bytecode.read(reinterpret_cast<char*>(&descriptorCount), sizeof(descriptorCount));
    boost::archive::binary_iarchive ia(bytecode, boost::archive::archive_flags::no_header);
    for (uint16_t i = 0; i < descriptorCount; i++) {
        OBlockDesc temp;
        ia >> temp;
        oblockDescs.push_back(temp);
    }
}

void BytecodeProcessor::loadLiterals() {
    uint16_t poolSize;
    bytecode.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));
    boost::archive::binary_iarchive ia(bytecode, boost::archive::archive_flags::no_header);
    for (uint16_t i = 0; i < poolSize; i++) {
        BlsType literal;
        ia >> literal;
        literalPool.push_back(literal);
    }
}

void BytecodeProcessor::loadInstructions() {
    OPCODE code;
    while (bytecode.read(reinterpret_cast<char*>(&code), sizeof(code))) {
        switch (code) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: {
            #define ARGUMENT(arg, type) \
                type arg; \
                bytecode.read(reinterpret_cast<char*>(&arg), sizeof(type));
            #define OPCODE_END(code, args...) \
                instructions.push_back(std::make_unique<INSTRUCTION::code>(INSTRUCTION::code{{OPCODE::code}, args})); \
                break; \
            } 
            #include "include/OPCODES.LIST"
            #undef OPCODE_BEGIN
            #undef ARGUMENT
            #undef OPCODE_END
            default:
                throw std::runtime_error("INVALID OPCODE");
            break;
        }
    }
}