#pragma once
#include "bytecode_processor.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>

class BytecodeWriter {
    public:
        void loadMnemonicBytecode(const std::string& filename);
        void writeMetadata(std::ostream& stream, boost::archive::binary_oarchive& oa);
        void writeHeader(std::ostream& stream, boost::archive::binary_oarchive& oa);
        void writeLiteralPool(std::ostream& stream, boost::archive::binary_oarchive& oa);
        void writeBody(std::ostream& stream);
        void convertToBinary(std::ostream& stream = std::cout);

    private:
        std::string loadJSON(std::string jsonDelimiter);
        template<bool Parse = true, typename T>
        void writeArg(T arg);
        template<bool Parse = true, typename... Args>
        void writeArgs(Args... args);
        void writeCALL(uint16_t address, uint8_t argc);
        void writeEMIT(uint8_t signal);
        void writePUSH(uint8_t index);
        void writeMKTYPE(uint8_t index, uint8_t type);
        void writeSTORE(uint8_t index);
        void writeLOAD(uint8_t index);
        void writeMTRAP(uint16_t callnum);
        void writeTRAP(uint16_t callnum, uint8_t argc);
        /* C style overloads of specialized functions to get around xmacro error in if constexpr for uncompiled branches */
        void writeCALL(...) { throw std::runtime_error("CALL PARSING OUT OF DATE"); }
        void writeEMIT(...) { throw std::runtime_error("EMIT PARSING OUT OF DATE"); }
        void writePUSH(...) { throw std::runtime_error("PUSH PARSING OUT OF DATE"); }
        void writeMKTYPE(...) { throw std::runtime_error("MKTYPE PARSING OUT OF DATE"); }
        void writeSTORE(...) { throw std::runtime_error("STORE PARSING OUT OF DATE"); }
        void writeLOAD(...) { throw std::runtime_error("LOAD PARSING OUT OF DATE"); }
        void writeMTRAP(...) { throw std::runtime_error("MTRAP PARSING OUT OF DATE"); }
        void writeTRAP(...) { throw std::runtime_error("TRAP PARSING OUT OF DATE"); }

        std::stringstream mnemonicBytecode;
        std::ostream* outputStream;
        std::unordered_map<BlsType, uint8_t> literalPool;
        std::unordered_map<std::string, std::pair<uint16_t, std::vector<std::string>>> functionSymbols;
        std::unordered_map<std::string, uint8_t> currentFunctionSymbols;
};
