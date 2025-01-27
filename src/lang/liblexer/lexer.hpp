#pragma once
#include "token.hpp"
#include "char_stream.hpp"
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
            Token lexInteger();
            Token lexFloat();
            Token lexString();
            Token lexOperator();
            Token lexComment();
    };

}