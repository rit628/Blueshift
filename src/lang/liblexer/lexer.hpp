#pragma once
#include "token.hpp"
#include "char_stream.hpp"
#include <cstddef>
#include <exception>
#include <sstream>
#include <string>
#include <vector>
#include <boost/regex.hpp>

namespace BlsLang {

    class Lexer {
        public:
            std::vector<Token> lex(const std::string& input);

        private:
            CharStream cs;

            Token lexToken();
            Token lexIdentifier();
            Token lexNumber();
            Token lexString();
            Token lexComment();
            Token lexOperator();
    };

    class LexException : public std::exception {
        public:
            explicit LexException(const std::string& message, size_t line, size_t col) {
                std::ostringstream os;
                os << "Ln " << line << ", Col " << col << ": " << message;
                this->message = os.str();
            }
        
            const char* what() const noexcept override { return message.c_str(); }

        private:
            std::string message;
    };

}