#pragma once
#include "bytecode_processor.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>

class BytecodeWriter {
    public:
        void loadMnemonicBytecode(const std::string& filename);
        void writeHeader(std::ostream& stream, boost::archive::binary_oarchive& oa);
        void writeLiteralPool(std::ostream& stream, boost::archive::binary_oarchive& oa);
        void writeBody(std::ostream& stream);
        void convertToBinary(std::ostream& stream = std::cout);

    private:
        std::stringstream mnemonicBytecode;
};
