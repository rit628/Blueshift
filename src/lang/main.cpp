#include "libinterpreter/interpreter.hpp"
#include "liblexer/lexer.hpp"
#include "libparser/parser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main() {
    // sample engine
    BlsLang::Lexer lexer;
    std::string test = \
    R"(a bunch of text on multiple
    lines with some // comments
    that are //singleline and
    multiline as follows /* some multiline
    comment that finishes just
    about here */ now after that lest test some
    strings like this one "some text \n \t " now
    time for some numbers 1 12 -12 decimals 
    too 1.2 .1 -1.2 -.3 -0.5 hooray now time for 
    the ops <= >= != == += -= *= /= ^= %= ++ -- && || . () $ yipee!)";
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