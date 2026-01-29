#include "bytecode_writer.hpp"
#include "Serialization.hpp"
#include "bytecode_processor.hpp"
#include "bls_types.hpp"
#include "bytecode_serializer.hpp"
#include "opcodes.hpp"
#include "traps.hpp"
#include <cstdint>
#include <memory>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
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

template<typename... Args>
void BytecodeWriter::parseArgs(Args&... args) {
    ((mnemonicBytecode >> args), ...);
}

void BytecodeWriter::loadMnemonicBytecode(const std::string& filename) {
    std::fstream input;
    input.open(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    mnemonicBytecode << input.rdbuf();
}

void BytecodeWriter::writeMetadata() {
    std::string buf;
    std::getline(mnemonicBytecode, buf); // skip METADATA_BEGIN
    auto metadataJSON = loadJSON("HEADER_BEGIN");
    functionSymbols = value_to<decltype(functionSymbols)>(parse(metadataJSON));
    bs->writeMetadata(functionSymbols);
}

void BytecodeWriter::writeHeader() {
    auto taskDescJSON = loadJSON("LITERALS_BEGIN");
    auto descriptors = value_to<std::vector<TaskDescriptor>>(parse(taskDescJSON));
    bs->writeHeader(descriptors);
}

void BytecodeWriter::writeLiteralPool() {
    auto poolJSON = loadJSON("BYTECODE_BEGIN");
    auto pool = value_to<std::vector<BlsType>>(parse(poolJSON));
    uint8_t index = 0;
    for (auto&& literal : pool) {
        literalPool[literal] = index++;
    }
    bs->writeLiteralPool(pool);
}

void BytecodeWriter::parseCALL(uint16_t& address, uint8_t& argc) {
    std::string functionName;
    mnemonicBytecode >> functionName;
    address = functionSymbols.at(functionName).first;
    parseArgs(argc);
}

void BytecodeWriter::parseEMIT(uint8_t& signal) {
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
}

void BytecodeWriter::parsePUSH(uint8_t& index) {
    std::string buf;
    std::getline(mnemonicBytecode, buf);
    auto literal = value_to<BlsType>(parse(buf));
    index = literalPool.at(literal);
}

void BytecodeWriter::parseMKTYPE(uint8_t& index, uint8_t& type) {
    std::string buf;
    mnemonicBytecode >> buf;
    index = currentFunctionSymbols.at(buf);
    mnemonicBytecode >> buf;
    type = static_cast<uint8_t>(getTypeFromName(buf));
}

void BytecodeWriter::parseSTORE(uint8_t& index) {
    std::string buf;
    mnemonicBytecode >> buf;
    index = currentFunctionSymbols.at(buf);
}

void BytecodeWriter::parseLOAD(uint8_t& index) {
    std::string buf;
    mnemonicBytecode >> buf;
    index = currentFunctionSymbols.at(buf);
}

void BytecodeWriter::parseMTRAP(uint16_t& callnum) {
    using namespace BlsTrap;
    std::string buf;
    mnemonicBytecode >> buf;
    callnum = static_cast<uint16_t>(getMTrapFromName(buf));
}

void BytecodeWriter::parseTRAP(uint16_t& callnum, uint8_t& argc) {
    using namespace BlsTrap;
    std::string buf;
    mnemonicBytecode >> buf;
    callnum = static_cast<uint16_t>(getTrapFromName(buf));
    parseArgs(argc);
}

void BytecodeWriter::writeBody() {
    std::string buf;
    while (mnemonicBytecode >> buf) {
        if (false) { } // hack to force short circuiting and invalid input checking
        #define OPCODE_BEGIN(code) \
        else if (buf == #code) { \
            constexpr OPCODE c = OPCODE::code; \
            auto instruction = create##code();
        #define ARGUMENT(arg, type) \
            type& arg = instruction->arg;
        #define OPCODE_END(code, args...) \
            if constexpr (c == OPCODE::CALL) { \
                parseCALL(args); \
            } \
            else if constexpr (c == OPCODE::EMIT) { \
                parseEMIT(args); \
            } \
            else if constexpr (c == OPCODE::PUSH) { \
                parsePUSH(args); \
            } \
            else if constexpr (c == OPCODE::MKTYPE) { \
                parseMKTYPE(args); \
            } \
            else if constexpr (c == OPCODE::STORE) { \
                parseSTORE(args); \
            } \
            else if constexpr (c == OPCODE::LOAD) { \
                parseLOAD(args); \
            } \
            else if constexpr (c == OPCODE::MTRAP) { \
                parseMTRAP(args); \
            } \
            else if constexpr (c == OPCODE::TRAP) { \
                parseTRAP(args); \
            } \
            else { \
                parseArgs(args); \
            } \
            bs->writeInstruction(*instruction); \
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
    bs = std::make_unique<BytecodeSerializer>(stream);
    writeMetadata();
    writeHeader();
    writeLiteralPool();
    writeBody();
}