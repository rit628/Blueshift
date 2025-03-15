#include "bytecode_writer.hpp"

int main(int argc, char** argv) {
    if(argc < 2){
        std::cerr << "invalid number of arguments provided" << std::endl;
        return 1; 
    }
    BytecodeWriter bsmp;
    bsmp.loadMnemonicBytecode("./samples/bsm/" + std::string(argv[1]));
    bsmp.convertToBinary();
}