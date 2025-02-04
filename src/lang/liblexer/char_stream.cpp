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

bool CharStream::peek(std::initializer_list<std::reference_wrapper<const boost::regex>> patterns) {
    std::string currChar;
    auto initIdx = ss.tellg();
    for (auto&& pattern : patterns) {
        currChar = ss.peek();
        if (ss.eof() || !boost::regex_match(currChar, pattern.get())) {
            ss.seekg(initIdx); // return to start
            return false;
        }
        ss.seekg(1, ss.cur);
    }
    ss.seekg(initIdx); // return to start
    return true;
}

bool CharStream::match(std::initializer_list<std::reference_wrapper<const boost::regex>> patterns) {
    if (!peek(patterns)) return false;
    
    char currChar;
    for (auto&& pattern : patterns) {
        currChar = ss.get();
        col++;
        currToken += currChar;
        if (currChar == '\n') {
            line++;
            col = 1;
        }
    }
    return true;
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