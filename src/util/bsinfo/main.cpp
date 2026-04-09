#include <fstream>
#include <string>
#include "compiler.hpp"
#include "bytecode_printer.hpp"

int main(int argc, char** argv) {
    std::stringstream compiledSrc;
    BlsLang::Compiler compiler;
    if (argc < 2) {
        std::string source{(std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>()};
        compiler.compileSource(source, compiledSrc);
    }
    else {
        compiler.compileFile(argv[1], compiledSrc);
    }
    BytecodePrinter bsmp;
    bsmp.loadBytecode(compiledSrc);
    auto* os = &std::cout;
    std::ofstream outfile;
    if (argc == 3) {
        outfile = std::ofstream(argv[2]);
        os = &outfile;
    }
    bsmp.setOutputStream(*os);
    *os << "Metadata:" << std::endl;
    bsmp.printMetadata();
    *os << "Header Data:" << std::endl;
    bsmp.printHeader();
}