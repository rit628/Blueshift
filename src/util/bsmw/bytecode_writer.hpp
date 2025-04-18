#pragma once
#include "libbytecode/bytecode_processor.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

class BytecodeWriter {
    public:
        void loadMnemonicBytecode(const std::string& filename);
        void writeHeader(std::ostream& stream = std::cout);
        void writeLiteralPool(std::ostream& stream = std::cout);
        void writeBody(std::ostream& stream = std::cout);
        void convertToBinary(std::ostream& stream = std::cout);

    private:
        std::stringstream mnemonicBytecode;
};