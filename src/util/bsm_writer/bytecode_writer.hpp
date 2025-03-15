#pragma once
#include "libbytecode/bytecode_processor.hpp"
#include <cstdint>
#include <fstream>

class BytecodeWriter {
    public:
        void loadMnemonicBytecode(const std::string& filename);
        void convertToBinary();

    private:
        template<typename... Args>
        void printArgs(Args... args);

        std::fstream mnemonicBytecode;
};