#include "char_stream.hpp"
#include <cstddef>
#include <cstdio>

using namespace BlsLang;

void CharStream::loadSource(const std::string& input) {
    line = 1;
    col = 1;
    ss.clear();
    ss.str(input);
    resetToken();
}

void CharStream::resetToken() {
    tokenLine = line;
    tokenCol = col;
    currToken.clear();
}

Token CharStream::emit(Token::Type type) {
    size_t startIdx = static_cast<size_t>(ss.tellg()) - currToken.size();
    Token t(type, currToken, startIdx, tokenLine, tokenCol);
    resetToken();
    return t;
}