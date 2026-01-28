#include "bytecode_writer.hpp"
#include "Serialization.hpp"
#include "bytecode_processor.hpp"
#include "DynamicMessage.hpp"
#include "bls_types.hpp"
#include "opcodes.hpp"
#include "traps.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
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

std::string BytecodeWriter::loadJSON(std::string jsonDelimiter) {
    std::string buf, result;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != jsonDelimiter)) {
        result += buf;
    }
    return result;
}

template<bool Parse, typename T>
void BytecodeWriter::writeArg(T arg) {
    if constexpr (Parse) {
        mnemonicBytecode >> arg;
    }
    outputStream->write(reinterpret_cast<const char *>(&arg), sizeof(T));
}

template<bool Parse, typename... Args>
void BytecodeWriter::writeArgs(Args... args) {
    (writeArg<Parse>(args), ...);
}

void BytecodeWriter::loadMnemonicBytecode(const std::string& filename) {
    std::fstream input;
    input.open(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    mnemonicBytecode << input.rdbuf();
}

void BytecodeWriter::writeMetadata(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf;
    std::getline(mnemonicBytecode, buf); // skip METADATA_BEGIN
    auto metadataJSON = loadJSON("HEADER_BEGIN");
    functionSymbols = value_to<decltype(functionSymbols)>(parse(metadataJSON));
    uint32_t metadataEnd = 0;
    stream.seekp(sizeof(metadataEnd));
    std::unordered_map<uint16_t, std::pair<std::string, std::vector<std::string>&>> functionMetadata;
    for (auto&& [name, metadata] : functionSymbols) {
        functionMetadata.emplace(metadata.first, std::make_pair(name, std::ref(metadata.second)));
    }
    oa << functionMetadata;
    metadataEnd = stream.tellp();
    stream.seekp(0);
    stream.write(reinterpret_cast<const char *>(&metadataEnd), sizeof(metadataEnd));
    stream.seekp(metadataEnd);
}

void BytecodeWriter::writeHeader(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    auto taskDescJSON = loadJSON("LITERALS_BEGIN");
    array descArray = parse(taskDescJSON).get_array();
    uint16_t descSize = descArray.size();
    stream.write(reinterpret_cast<const char *>(&descSize), sizeof(descSize));
    for (auto&& desc : descArray) {
        oa << value_to<TaskDescriptor>(desc);
    }
}

void BytecodeWriter::writeLiteralPool(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    auto poolJSON = loadJSON("BYTECODE_BEGIN");
    array pool = parse(poolJSON).get_array();
    uint16_t poolSize = pool.size();
    stream.write(reinterpret_cast<const char *>(&poolSize), sizeof(poolSize));
    uint8_t index = 0;
    for (auto&& literal : pool) {
        auto value = value_to<BlsType>(literal);
        literalPool[value] = index++;
        oa << value;
    }
}

void BytecodeWriter::writeCALL(uint16_t address, uint8_t argc) {
    std::string functionName;
    mnemonicBytecode >> functionName;
    address = functionSymbols.at(functionName).first;
    writeArgs<false>(address);
    writeArgs(argc);
}

void BytecodeWriter::writeEMIT(uint8_t signal) {
    using enum BytecodeProcessor::SIGNAL;
    std::string signalString;
    mnemonicBytecode >> signalString;
    if (signalString == "START") {
        signal = static_cast<uint8_t>(START);
    }
    else if (signalString == "STOP") {
        signal = static_cast<uint8_t>(STOP);
    }
    else {
        signal = static_cast<uint8_t>(COUNT);
    }
    writeArgs<false>(signal);
}

void BytecodeWriter::writePUSH(uint8_t index) {
    std::string buf;
    std::getline(mnemonicBytecode, buf);
    auto literal = value_to<BlsType>(parse(buf));
    index = literalPool.at(literal);
    writeArgs<false>(index);
}

void BytecodeWriter::writeMKTYPE(uint8_t index, uint8_t type) {
    std::string buf;
    mnemonicBytecode >> buf;
    index = currentFunctionSymbols.at(buf);
    mnemonicBytecode >> buf;
    type = static_cast<uint8_t>(getTypeFromName(buf));
    writeArgs<false>(index, type);
}

void BytecodeWriter::writeSTORE(uint8_t index) {
    std::string buf;
    mnemonicBytecode >> buf;
    index = currentFunctionSymbols.at(buf);
    writeArgs<false>(index);
}

void BytecodeWriter::writeLOAD(uint8_t index) {
    std::string buf;
    mnemonicBytecode >> buf;
    index = currentFunctionSymbols.at(buf);
    writeArgs<false>(index);
}

void BytecodeWriter::writeMTRAP(uint16_t callnum) {
    using namespace BlsTrap;
    std::string buf;
    mnemonicBytecode >> buf;
    callnum = static_cast<uint16_t>(getMTrapFromName(buf));
    writeArgs<false>(callnum);
}

void BytecodeWriter::writeTRAP(uint16_t callnum, uint8_t argc) {
    using namespace BlsTrap;
    std::string buf;
    mnemonicBytecode >> buf;
    callnum = static_cast<uint16_t>(getTrapFromName(buf));
    writeArgs<false>(callnum);
    writeArgs(argc);
}

void BytecodeWriter::writeBody(std::ostream& stream) {
    std::string buf;
    outputStream = &stream; // quick and dirty; will fix shortly
    while (mnemonicBytecode >> buf) {
        if (false) { } // hack to force short circuiting and invalid input checking
        #define OPCODE_BEGIN(code) \
        else if (buf == #code) { \
            constexpr OPCODE c = OPCODE::code; \
            stream.write(reinterpret_cast<const char *>(&c), sizeof(c));
        #define ARGUMENT(arg, type) \
            type arg = type();
        #define OPCODE_END(code, args...) \
            if constexpr (c == OPCODE::CALL) { \
                writeCALL(args); \
            } \
            else if constexpr (c == OPCODE::EMIT) { \
                writeEMIT(args); \
            } \
            else if constexpr (c == OPCODE::PUSH) { \
                writePUSH(args); \
            } \
            else if constexpr (c == OPCODE::MKTYPE) { \
                writeMKTYPE(args); \
            } \
            else if constexpr (c == OPCODE::STORE) { \
                writeSTORE(args); \
            } \
            else if constexpr (c == OPCODE::LOAD) { \
                writeLOAD(args); \
            } \
            else if constexpr (c == OPCODE::MTRAP) { \
                writeMTRAP(args); \
            } \
            else if constexpr (c == OPCODE::TRAP) { \
                writeTRAP(args); \
            } \
            else { \
                writeArgs(args); \
            } \
        }
        #include "include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END
        else if (buf.ends_with(':')) { // task/procedure labels
            buf.pop_back();
            auto& symbolList = functionSymbols.at(buf).second;
            uint8_t index = 0;
            for (auto&& symbol : symbolList) {
                currentFunctionSymbols[symbol] = index++;
            }
            continue;
        }
        else if (buf.starts_with('[')) { // optional address enumeration
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
    writeMetadata(stream, oa);
    writeHeader(stream, oa);
    writeLiteralPool(stream, oa);
    writeBody(stream);
}