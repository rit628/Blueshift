#pragma once
#include "liblexer/token.hpp"
#include <cstddef>
#include <vector>
#include <string>
#include <concepts>

namespace BlsLang {

    template<typename T>
    concept TokenAttr = std::same_as<T, std::string>
                     || std::same_as<T, const char*>
                     || std::same_as<T, Token::Type>;

    class TokenStream {
        public:
            TokenStream() {}
            TokenStream(std::vector<Token> input) : ts(std::move(input)) {}

            // Looks ahead in stream (without consuming tokens) for tokens matching given patterns
            template<TokenAttr... Args>
            bool peek(Args... patterns);
            // Looks ahead in stream for tokens matching given patterns and consumes matching tokens
            template<TokenAttr... Args>
            bool match(Args... patterns);
            const Token& at(int offset) const;
            void setStream(std::vector<Token>& newStream);
            size_t getLine() const;
            size_t getColumn() const;
            bool empty() const { return outOfRange(0); }

        private:
            template<TokenAttr T>
            bool peekPattern(T pattern, size_t index);
            bool outOfRange(int offset) const { return (ts.size() <= (index + offset)) || (index + offset) < 0; }

            std::vector<Token> ts;
            size_t index = 0;
    };

    template<TokenAttr... Args>
    inline bool TokenStream::peek(Args... patterns) {
        size_t index = 0;
        return (peekPattern(std::forward<Args>(patterns), index++) && ...);
    }

    template<TokenAttr... Args>
    inline bool TokenStream::match(Args... patterns) {
        if (!peek(patterns...)) return false;
        size_t numPatterns = sizeof...(patterns);
        index += numPatterns;
        return true;
    }

    template<TokenAttr T>
    bool TokenStream::peekPattern(T pattern, size_t index) {
        if (outOfRange(index)) return false;

        if constexpr (std::is_same<T, const char*>() || std::is_same<T, std::string>()) {
            return ts.at(index + this->index).getLiteral() == pattern;
        }
        else {
            return ts.at(index + this->index).getType() == pattern;
        }
    }

}