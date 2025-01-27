#include "lexer.hpp"
#include "include/token_definitions.hpp"
#include <string>
#include <vector>
#include <boost/regex.hpp>

using namespace BlsLang;

std::vector<Token> Lexer::lex(const std::string& input) {
    cs.loadSource(input);
    std::vector<Token> tokens;
    while (!cs.empty()) {
        while (cs.match({WHITESPACE}));
        cs.resetToken();
        tokens.push_back(lexToken());
    }
    return tokens;
}

Token Lexer::lexToken() {
    // temporary for now so everything can compile for sample engine
    while (cs.match({IDENTIFIER_START}));
    return cs.emit(Token::Type::OPERATOR);
}

Token Lexer::lexIdentifier() {

}

Token Lexer::lexInteger() {

}

Token Lexer::lexFloat() {

}

Token Lexer::lexString() {

}

Token Lexer::lexOperator() {

}

Token Lexer::lexComment() {

}
