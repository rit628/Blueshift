#pragma once
#include "bytecode_processor.hpp"
#include "Serialization.hpp"
#include "opcodes.hpp"
#include "DynamicMessage.hpp"
#include "bls_types.hpp"
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

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::readMetadata(std::istream& bytecode, boost::archive::binary_iarchive& ia) {
    uint32_t metadataEnd;
    bytecode.read(reinterpret_cast<char*>(&metadataEnd), sizeof(metadataEnd));
    if constexpr (SkipMetadata) {
        bytecode.seekg(metadataEnd);
    }
    else {
        ia >> functionMetadata;
    }
}

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::readHeader(std::istream& bytecode, boost::archive::binary_iarchive& ia) {
    uint16_t descriptorCount;
    bytecode.read(reinterpret_cast<char*>(&descriptorCount), sizeof(descriptorCount));
    for (uint16_t i = 0; i < descriptorCount; i++) {
        TaskDescriptor temp;
        ia >> temp;
        taskDescs.push_back(temp);
    }
}

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::loadLiterals(std::istream& bytecode, boost::archive::binary_iarchive& ia) {
    uint16_t poolSize;
    bytecode.read(reinterpret_cast<char*>(&poolSize), sizeof(poolSize));
    for (uint16_t i = 0; i < poolSize; i++) {
        BlsType literal;
        ia >> literal;
        literalPool.push_back(literal);
    }
}

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::loadInstructions(std::istream& bytecode) {
    OPCODE code;
    while (bytecode.read(reinterpret_cast<char*>(&code), sizeof(code))) {
        switch (code) {
            #define OPCODE_BEGIN(code) \
            case OPCODE::code: {
            #define ARGUMENT(arg, type) \
                type arg; \
                bytecode.read(reinterpret_cast<char*>(&arg), sizeof(type));
            #define OPCODE_END(code, args...) \
                instructions.push_back(create##code(args)); \
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

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::loadBytecode(std::istream& bytecode) {
    if (bytecode.bad()) {
        throw std::runtime_error("Bad bytecode stream provided.");
    }
    boost::archive::binary_iarchive ia(bytecode, boost::archive::archive_flags::no_header);
    readMetadata(bytecode, ia);
    readHeader(bytecode, ia);
    loadLiterals(bytecode, ia);
    loadInstructions(bytecode);
}

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::loadBytecode(const std::string& filename) {
    auto bytecode = std::ifstream(filename, std::ios::binary);
    if (!bytecode.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    loadBytecode(bytecode);
}

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::loadBytecode(const std::vector<char>& bytecode) {
    auto bytecodeStream = std::istringstream({bytecode.data(), bytecode.size()}, std::ios::binary);
    loadBytecode(bytecodeStream);
}

template<bool SkipMetadata>
inline void BytecodeProcessor<SkipMetadata>::dispatch() {
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