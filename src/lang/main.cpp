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
    file.open("test/lang/samples/function.blu");
    std::stringstream ss;
    ss << file.rdbuf();
    auto ret = lexer.lex(ss.str());
    std::cout << ret.size() << std::endl;
    for (auto&& i : ret) {
        std::cout << i.getTypeName() << " " << i.getLiteral() << " @ " << i.getLineNum() << ":" << i.getColNum() << " or " << i.getAbsIdx() <<  std::endl;
    }

    BlsLang::Parser parser;
    std::cout << *parser.parse(ret);
}