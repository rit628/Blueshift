#pragma once
#include "token.hpp"
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <sstream>
#include <boost/regex.hpp>

namespace BlsLang {
    class CharStream {
        public:
            CharStream() {}
            CharStream(const std::string& input) : ss(input) {}

            void loadSource(const std::string& input);
            // Looks ahead in stream (without consuming characters) for characters matching given patterns
            bool peek(std::initializer_list<std::reference_wrapper<const boost::regex>> patterns);
            // Looks ahead in stream for characters matching given patterns and consumes matching characters
            bool match(std::initializer_list<std::reference_wrapper<const boost::regex>> patterns);
            void resetToken();
            // Creates a token with the currently consumed characters
            Token emit(Token::Type type);
            bool empty() { return ss.eof() || ss.tellg() == ss.view().size(); }
            const std::string& getTokenState() const { return currToken; }
        private:
            std::istringstream ss;
            std::string currToken = "";
            size_t line = 0, col = 0, tokenLine = 0, tokenCol = 0;
    };
}