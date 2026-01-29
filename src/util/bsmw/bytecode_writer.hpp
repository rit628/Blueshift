#pragma once
#include "bytecode_processor.hpp"
#include "bytecode_serializer.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>

class BytecodeWriter {
    public:
        void loadMnemonicBytecode(const std::string& filename);
        void convertToBinary(std::ostream& stream = std::cout);

    private:
        void writeMetadata();
        void writeHeader();
        void writeLiteralPool();
        void writeBody();

        std::string loadJSON(std::string jsonDelimiter);
        template<typename... Args>
        void parseArgs(Args&... args);

        void parseCALL(uint16_t& address, uint8_t& argc);
        void parseEMIT(uint8_t& signal);
        void parsePUSH(uint8_t& index);
        void parseMKTYPE(uint8_t& index, uint8_t& type);
        void parseSTORE(uint8_t& index);
        void parseLOAD(uint8_t& index);
        void parseMTRAP(uint16_t& callnum);
        void parseTRAP(uint16_t& callnum, uint8_t& argc);
        /* C style overloads of specialized functions to get around xmacro error in if constexpr for uncompiled branches */
        void parseCALL(...) { throw std::runtime_error("CALL PARSING OUT OF DATE"); }
        void parseEMIT(...) { throw std::runtime_error("EMIT PARSING OUT OF DATE"); }
        void parsePUSH(...) { throw std::runtime_error("PUSH PARSING OUT OF DATE"); }
        void parseMKTYPE(...) { throw std::runtime_error("MKTYPE PARSING OUT OF DATE"); }
        void parseSTORE(...) { throw std::runtime_error("STORE PARSING OUT OF DATE"); }
        void parseLOAD(...) { throw std::runtime_error("LOAD PARSING OUT OF DATE"); }
        void parseMTRAP(...) { throw std::runtime_error("MTRAP PARSING OUT OF DATE"); }
        void parseTRAP(...) { throw std::runtime_error("TRAP PARSING OUT OF DATE"); }

        std::stringstream mnemonicBytecode;
        std::unique_ptr<BytecodeSerializer> bs = nullptr;
        std::unordered_map<BlsType, uint8_t> literalPool;
        std::unordered_map<std::string, std::pair<uint16_t, std::vector<std::string>>> functionSymbols;
        std::unordered_map<std::string, uint8_t> currentFunctionSymbols;
};
