#include "Serialization.hpp"
#include "compiler.hpp"
#include <fstream>
#include <stdexcept>

int main(int argc, char** argv) {

    // sample engine
    BlsLang::Compiler compiler;
    if (argc < 2) {
        throw std::runtime_error("No input file provided");
    }
    auto out = std::ofstream("./samples/bsm/out.bsm", std::ios::out | std::ios::binary);
    compiler.compileFile(argv[1], out);
    return 0;
}