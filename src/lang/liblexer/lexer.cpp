#include "lexer.hpp"
#include "include/token_definitions.hpp"
#include "token.hpp"
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
    if (cs.peek({IDENTIFIER_START})) {
        return lexIdentifier();
    }
    else if (cs.peek({NUMERIC_DIGIT}) ||
             cs.peek({NEGATIVE_SIGN, NUMERIC_DIGIT}) ||
             cs.peek({NEGATIVE_SIGN, DECIMAL_POINT}) ||
             cs.peek({DECIMAL_POINT, NUMERIC_DIGIT})) {
        return lexNumber();
    }
    else if (cs.peek({STRING_QUOTE})) {
        return lexString();
    }
    else if (cs.peek({COMMENT_SLASH, COMMENT_SLASH}) ||
             cs.peek({COMMENT_SLASH, COMMENT_STAR})) {
        return lexComment();
    }
    else {
        return lexOperator();
    }
}

Token Lexer::lexIdentifier() {
    cs.match({IDENTIFIER_START});
    while (cs.match({IDENTIFIER_END}));
    return cs.emit(Token::Type::IDENTIFIER);
}

Token Lexer::lexNumber() {

}

Token Lexer::lexString() {

}

Token Lexer::lexComment() {

}

Token Lexer::lexOperator() {

}
