#include "liblexer/lexer.hpp"
#include <iostream>
#include <string>

int main() {
    // sample engine
    BlsLang::Lexer lexer;
    std::string test = "a bunch of \n text";
    auto ret = lexer.lex(test);
    std::cout << ret.size() << std::endl;
    for (auto&& i : ret) {
        std::cout << i.getLiteral() << " @ " << i.getLineNum() << ":" << i.getColNum() << " or " << i.getAbsIdx() <<  std::endl;
    }
}