#include "bytecode_writer.hpp"
#include <fstream>

int main(int argc, char** argv) {
    std::string input;
    std::ofstream output;
    std::ostream* stream = &std::cout;
    if (argc < 2 || argc > 3) {
        std::cerr << "invalid number of arguments provided" << std::endl;
        return 1; 
    }
    input = "./samples/bsm/" + std::string(argv[1]);
    if (argc == 3) {
        output.open("./samples/bsm/" + std::string(argv[2]));
        stream = &output;
    }
    BytecodeWriter bsmp;
    bsmp.loadMnemonicBytecode(input);
    bsmp.convertToBinary(*stream);
}