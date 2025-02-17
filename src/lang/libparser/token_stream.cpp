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

size_t TokenStream::getLine() const {
    if (ts.empty()) return 0;
    auto idx = outOfRange(index) ? ts.size() - 1 : index;
    return ts.at(idx).getLineNum();
}

size_t TokenStream::getColumn() const {
    if (ts.empty()) return 0;
    auto idx = outOfRange(index) ? ts.size() - 1 : index;
    return ts.at(idx).getColNum();
}
