#include "token_stream.hpp"
#include <algorithm>
#include <cstddef>
#include <stdexcept>

using namespace BlsLang;

const Token& TokenStream::at(int offset) const {
    if (outOfRange(offset)) {
        throw std::runtime_error("Out of range access for token stream.");
    }
    return ts.at(index + offset);
}

void TokenStream::setStream(std::vector<Token>& newStream) {
    ts = std::move(newStream);
    // remove comments
    ts.erase(std::remove_if(ts.begin(), ts.end(), [](const Token& t) {
        return t.getType() == Token::Type::COMMENT;
    }), ts.end());
}

size_t TokenStream::getLine(bool fromLastToken) const {
    if (ts.empty()) return 0;
    auto idx = std::min(index - static_cast<int>(fromLastToken), ts.size() - 1);
    auto& token = ts.at(idx);
    return (fromLastToken) ? token.getLineEnd() : token.getLineStart();
}

size_t TokenStream::getColumn(bool fromLastToken) const {
    if (ts.empty()) return 0;
    auto idx = std::min(index - static_cast<int>(fromLastToken), ts.size() - 1);
    auto& token = ts.at(idx);
    return (fromLastToken) ? token.getColEnd() : token.getColStart();
}

std::pair<size_t, size_t> TokenStream::getLocation(bool fromLastToken) const {
    return {getLine(fromLastToken), getColumn(fromLastToken)};
}