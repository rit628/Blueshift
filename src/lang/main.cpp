#include "liblexer/lexer.hpp"
#include <iostream>
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
    strings like this one "some text \n \t ")";
    auto ret = lexer.lex(test);
    std::cout << ret.size() << std::endl;
    for (auto&& i : ret) {
        std::cout << i.getTypeName() << " " << i.getLiteral() << " @ " << i.getLineNum() << ":" << i.getColNum() << " or " << i.getAbsIdx() <<  std::endl;
    }
}