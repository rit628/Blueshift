#pragma once
#include "libbytecode/bytecode_processor.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>

class BytecodeWriter {
    public:
        void loadMnemonicBytecode(const std::string& filename);
        void convertToBinary(std::ostream& stream = std::cout);

    private:
        std::fstream mnemonicBytecode;
};