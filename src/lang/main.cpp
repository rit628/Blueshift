#include "Serialization.hpp"
#include "compiler.hpp"
#include "optimizer.hpp"
#include <fstream>
#include <stdexcept>

int main(int argc, char** argv) {

    // sample engine
    BlsLang::Compiler compiler;
    std::string src = R"(
        task printString(TIMER_TEST T) {
            println("A string.");
        }

        task printAppendedString(TIMER_TEST T) {
            string s = "A string.";
            s += " Another string.";
            println(s);
        }

        setup() {
            TIMER_TEST T1 = "CTL::PWR-0";
            printString(T1);
            printAppendedString(T1);
        }
    )";
    
    auto out = std::ofstream("./samples/bsm/out.bsm", std::ios::out | std::ios::binary);

    compiler.compileFile(argv[1], out);

    BlsLang::Optimizer opt; 
    opt.loadBytecode("./samples/bsm/out.bsm"); 
    opt.optimize(); 
    return 0;
}