#include "include/Common.hpp"
#include "libcompiler/compiler.hpp"
#include <fstream>
#include <stdexcept>

int main() {

    // sample engine
    BlsLang::Compiler compiler;
    std::string src = R"(
        oblock printString(TIMER_TEST T) {
            println("A string.");
        }

        oblock printAppendedString(TIMER_TEST T) {
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
    if (!out.is_open()) {
        throw std::runtime_error("FUCK YOU");
    }
    compiler.compileSource(src, out);
    return 0;
}