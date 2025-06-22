#include "libcompiler/compiler.hpp"

int main() {
    // sample engine
    BlsLang::Compiler compiler;
    compiler.compileFile("./samples/src/simple.blu");
    return 0;
}