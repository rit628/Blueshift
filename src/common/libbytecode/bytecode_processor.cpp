#include "bytecode_processor.hpp"
#include "include/Common.hpp"
#include "opcodes.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtype/bls_types.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

void BytecodeProcessor::loadBytecode(std::istream& bytecode) {
    if (bytecode.bad()) {
        throw std::runtime_error("Bad bytecode stream provided.");
    }
    boost::archive::binary_iarchive ia(bytecode, boost::archive::archive_flags::no_header);
    readHeader(bytecode, ia);
    loadLiterals(bytecode, ia);
    loadInstructions(bytecode, ia);
}

void BytecodeProcessor::loadBytecode(const std::string& filename) {
    auto bytecode = std::ifstream(filename, std::ios::binary);
    if (!bytecode.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    loadBytecode(bytecode);
}

void BytecodeProcessor::loadBytecode(const std::vector<char>& bytecode) {
    auto bytecodeStream = std::istringstream({bytecode.data(), bytecode.size()}, std::ios::binary);
    loadBytecode(bytecodeStream);
}

void BytecodeProcessor::dispatch() {
    while (instruction != instructions.size()) {
        auto& instructionStruct = instructions[instruction];
        instruction++;
        switch (instructionStruct->opcode) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: { \
                auto& resolvedInstruction [[ maybe_unused ]] = reinterpret_cast<INSTRUCTION::code&>(*instructionStruct);
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
            case SIGNAL::START:
            break;
            case SIGNAL::STOP:
                goto exit;
            break;
            default:
            break;
        }
    }
    exit: ;
}

void BytecodeProcessor::readHeader(std::istream& bytecode, boost::archive::binary_iarchive& ia) {
    uint16_t descriptorCount;
    bytecode.read(reinterpret_cast<char*>(&descriptorCount), sizeof(descriptorCount));
    for (uint16_t i = 0; i < descriptorCount; i++) {
        TaskDescriptor temp;
        ia >> temp;
        taskDescs.push_back(temp);
    }
}

void BytecodeProcessor::loadLiterals(std::istream& bytecode, boost::archive::binary_iarchive& ia) {
    uint16_t poolSize;
    bytecode.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));
    for (uint16_t i = 0; i < poolSize; i++) {
        BlsType literal;
        ia >> literal;
        literalPool.push_back(literal);
    }
}

void BytecodeProcessor::loadInstructions(std::istream& bytecode, boost::archive::binary_iarchive& ia) {
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