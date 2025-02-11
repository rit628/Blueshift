#pragma once
#include "token.hpp"
#include <concepts>
#include <cstddef>
#include <sstream>
#include <boost/regex.hpp>

namespace BlsLang {

    template<typename T>
    concept CharacterPattern = std::same_as<T, char>
                            || std::same_as<T, boost::regex>;

    class CharStream {
        public:
            CharStream() {}
            CharStream(const std::string& input) : ss(input) {}

            void loadSource(const std::string& input);
            // Looks ahead in stream (without consuming characters) for characters matching given patterns
            template<CharacterPattern... Args>
            bool peek(Args... patterns);
            // Looks ahead in stream for characters matching given patterns and consumes matching characters
            template<CharacterPattern... Args>
            bool match(Args... patterns);
            void resetToken();
            // Creates a token with the currently consumed characters
            Token emit(Token::Type type);
            bool empty() { return ss.eof() || ss.tellg() == ss.view().size(); }
            const std::string& getTokenState() const { return currToken; }
            size_t getLine() const { return line; }
            size_t getColumn() const { return col; }
        private:
            template<CharacterPattern T>
            bool peekPattern(T pattern);

            std::istringstream ss;
            std::string currToken = "";
            size_t line = 1, col = 1, tokenLine = 1, tokenCol = 1;
    };

    template<CharacterPattern... Args>
    inline bool CharStream::peek(Args... patterns) {
        size_t initIdx = ss.tellg();
        auto result = (peekPattern(std::forward<Args>(patterns)) && ...);
        ss.seekg(initIdx); // return to start
        return result;
    }

    template<CharacterPattern... Args>
    inline bool CharStream::match(Args... patterns) {
        if (!peek(patterns...)) return false;
        size_t numPatterns = sizeof...(patterns);
        char currChar;
        for (size_t i = 0; i < numPatterns; i++) {
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

    template<CharacterPattern T>
    inline bool CharStream::peekPattern(T pattern) {
        if constexpr (std::is_same<T, char>()) {
            if (ss.eof() || ss.peek() != pattern) {
                return false;
            }
        }
        else {
            std::string currChar; 
            currChar = ss.peek();
            if (ss.eof() || !boost::regex_match(currChar, pattern)) {
                return false;
            }
        }
        ss.seekg(1, ss.cur); // move forward in stream
        return true;
    }
}