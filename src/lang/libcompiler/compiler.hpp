#pragma once
#include "ast.hpp"
#include "liblexer/lexer.hpp"
#include "libparser/parser.hpp"
#include "libinterpreter/interpreter.hpp"
#include "token.hpp"
#include <memory>
#include <vector>

namespace BlsLang {

    class Compiler {
        public:
            void compileFile(const std::string& source);
            void compileSource(const std::string& source);
            auto& getOblocks() { return interpreter.getOblocks(); }
            auto& getDeviceDescriptors() { return interpreter.getDeviceDescriptors(); }
            auto& getOblockDescriptors() { return interpreter.getOblockDescriptors(); }
        
        private:
            std::vector<Token> tokens;
            std::unique_ptr<AstNode> ast;
            Lexer lexer;
            Parser parser;
            Interpreter interpreter;
    };

}