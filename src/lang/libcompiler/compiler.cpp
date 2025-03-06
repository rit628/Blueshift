#include "compiler.hpp"
#include <fstream>
#include <sstream>

using namespace BlsLang;

void Compiler::compileFile(const std::string& source) {
    std::ifstream file;
    file.open(source);
    if (!file.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    std::stringstream ss;
    ss << file.rdbuf();
    compileSource(ss.str());
}

void Compiler::compileSource(const std::string& source) {
    tokens = lexer.lex(source);
    ast = parser.parse(tokens);
    ast->accept(interpreter);
}