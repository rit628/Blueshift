#include <fstream>
#include <string>
#include "compiler.hpp"
#include "bytecode_printer.hpp"
#include "error_types.hpp"

int main(int argc, char** argv) {
    std::stringstream compiledSrc;
    BlsLang::Compiler compiler;
    auto* os = &std::cout;
    std::ofstream outfile;
    if (argc == 3) {
        outfile = std::ofstream(argv[2]);
        os = &outfile;
    }
    try {
        if (argc < 2) {
            std::string source{(std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>()};
            compiler.compileSource(source, compiledSrc);
        }
        else {
            compiler.compileFile(argv[1], compiledSrc);
        }
        BytecodePrinter bsmp;
        bsmp.loadBytecode(compiledSrc);
        bsmp.setOutputStream(*os);
        bsmp.printHeader();
    }
    catch (const BlsLang::SyntaxError& e) {
        *os << "Syntax Error: " <<  e.what() << std::endl;
    }
    catch (const BlsLang::SemanticError& e) {
        *os << "Semantic Error: " << e.what() << std::endl;
    }
}