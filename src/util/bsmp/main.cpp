#include "bytecode_printer.hpp"

int main(int argc, char** argv) {
    std::string input;
    std::ofstream output;
    std::ofstream jsonOutput; 

    std::ostream* stream = &std::cout;
    std::ostream* jsonStream = &std::cout; 

    if (argc < 2 || argc > 3) {
        std::cerr << "invalid number of arguments provided" << std::endl;
        return 1; 
    }
    input = "./samples/bsm/" + std::string(argv[1]);
    BytecodePrinter bsmp;
    bsmp.loadBytecode(input);
    if (argc == 3) {
        output.open("./samples/bsm/" + std::string(argv[2]));
        stream = &output;
    }
    bsmp.setOutputStream(*stream);
    bsmp.printAll();

    // Print the JSON data to be read by the file extension 

    BytecodePrinter jsonPrinter; 
    jsonPrinter.loadBytecode(input); 
    if(argc == 3){
        jsonOutput.open("./samples/bsm/visualData.json"); 
        jsonStream = &jsonOutput; 
    }

    jsonPrinter.setOutputStream(*jsonStream); 
    jsonPrinter.printHeader(); 
}