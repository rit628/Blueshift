#include "bytecode_printer.hpp"

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
    if(argc < 2){
        std::cerr << "invalid number of arguments provided" << std::endl;
        return 1; 
    }
    BytecodePrinter bsmp;
    bsmp.loadBytecode(input);
    bsmp.setOutputStream(*stream);
    bsmp.dispatch();
}