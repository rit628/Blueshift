#include "token_stream.hpp"
#include <cstddef>
#include <stdexcept>

using namespace BlsLang;

const Token& TokenStream::at(size_t offset) const {
    if (outOfRange(index)) {
        throw std::runtime_error("Out of range access for token stream.");
    }
    return ts.at(index + offset);
}
