#include "bytecode_printer.hpp"

int main(int argc, char** argv) {
    if(argc < 2){
        std::cerr << "invalid number of arguments provided" << std::endl;
        return 1; 
    }
    BytecodePrinter bsmp;
    bsmp.loadBytecode("./samples/bsm/" + std::string(argv[1]));
    bsmp.dispatch();
}