#include "bls_types.hpp"
#include "libanalyzer/analyzer.hpp"
#include "libinterpreter/interpreter.hpp"
#include "liblexer/lexer.hpp"
#include "libparser/parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int main() {
    // sample engine
    BlsLang::Lexer lexer;
    std::ifstream file;
    file.open("test/lang/samples/simple.blu");
    std::stringstream ss;
    ss << file.rdbuf();
    auto ret = lexer.lex(ss.str());
    std::cout << ret.size() << std::endl;
    for (auto&& i : ret) {
        std::cout << i.getTypeName() << " " << i.getLiteral() << " @ " << i.getLineNum() << ":" << i.getColNum() << " or " << i.getAbsIdx() <<  std::endl;
    }

    BlsLang::Parser parser;
    auto ast = parser.parse(ret);

    std::cout << *ast;
    
    BlsLang::Interpreter interpreter;
    ast->accept(interpreter);
    for (auto&& i : interpreter.getOblockDescriptors()) {
        std::cout << i.name << std::endl;
        for (auto&& j : i.binded_devices) {
            std::cout << j.device_name << std::endl;
            std::cout << j.controller << std::endl;
            std::cout << static_cast<int>(j.devtype) << std::endl;
            for (auto&& k : j.port_maps) {
                std::cout << k.first << std::endl;
                std::cout << k.second << std::endl;
            }
        }
    }

    return 0;
}