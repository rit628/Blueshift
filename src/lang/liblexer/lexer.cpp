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
        if (!cs.empty()) { // prevent attempts to lex empty token when EOF is reached
            tokens.push_back(lexToken());
        }
    }
    return tokens;
}

Token Lexer::lexToken() {
    if (cs.peek({IDENTIFIER_START})) {
        return lexIdentifier();
    }
    else if (cs.peek({NUMERIC_DIGIT}) ||
             cs.peek({NEGATIVE_SIGN, NUMERIC_DIGIT}) ||
             cs.peek({DECIMAL_POINT, NUMERIC_DIGIT}) ||
             cs.peek({NEGATIVE_SIGN, DECIMAL_POINT, NUMERIC_DIGIT})) {
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
    cs.match({NEGATIVE_SIGN}); // consume negative sign
    
    Token::Type tokenType = Token::Type::INTEGER;

    if (cs.match({ZERO, HEX_START, HEX_DIGIT})) { // hexadecmial literal
        while (cs.match({HEX_DIGIT}));
    }
    else if (cs.match({ZERO, BINARY_START, BINARY_DIGIT})) { // binary literal
        while (cs.match({BINARY_DIGIT}));
    }
    else if (!cs.peek({ZERO, DECIMAL_POINT}) && cs.match({ZERO})) { // octal literal
        while (cs.match({OCTAL_DIGIT}));
    }
    else { // decimal literal 
        while (cs.match({NUMERIC_DIGIT})); // consume digits before decimal point
        if (cs.match({DECIMAL_POINT, NUMERIC_DIGIT})) {
            tokenType = Token::Type::FLOAT;
            while (cs.match({NUMERIC_DIGIT})); // consume digits after decimal point
        }
    }

    return cs.emit(tokenType);
}

Token Lexer::lexString() {
    cs.match({STRING_QUOTE}); // consume opening quote
    while (!cs.match({STRING_QUOTE})) {
        if (cs.peek({ESCAPE_SLASH})) {  // consume escape sequence
            if (!cs.match({ESCAPE_SLASH, ESCAPE_CHARS})) {
                throw LexException("Invalid escape sequence.", cs.getLine(), cs.getColumn());
            }
        }
        else if (cs.empty() || !cs.match({STRING_LITERALS})) {  // consume string content
            throw LexException("Unterminated string literal.", cs.getLine(), cs.getColumn());
        }
    }
    return cs.emit(Token::Type::STRING);
}

Token Lexer::lexComment() {
    if (cs.match({COMMENT_SLASH, COMMENT_SLASH})) { // singleline comment
        while(cs.match({COMMENT_CONTENTS_SINGLELINE}));
    }
    else if (cs.match({COMMENT_SLASH, COMMENT_STAR})) { // multiline comment
        while (!cs.match({COMMENT_STAR, COMMENT_SLASH})) {
            if (cs.empty()) {
                throw LexException("Unterminated mutiline comment.", cs.getLine(), cs.getColumn());
            }
            cs.match({COMMENT_CONTENTS_MULTILINE});
        }
    }
    return cs.emit(Token::Type::COMMENT);
}

Token Lexer::lexOperator() {
    if (cs.match({OPERATOR_EQUALS_PREFIX, OPERATOR_EQUALS}) ||
        cs.match({OPERATOR_PLUS, OPERATOR_PLUS}) ||
        cs.match({OPERATOR_MINUS, OPERATOR_MINUS}) ||
        cs.match({OPERATOR_AND, OPERATOR_AND}) ||
        cs.match({OPERATOR_OR, OPERATOR_OR}) ||
        cs.match({OPERATOR_GENERIC})) {
        return cs.emit(Token::Type::OPERATOR);
    }
    throw LexException("Illegal character or end of file.", cs.getLine(), cs.getColumn());
}