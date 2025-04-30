#pragma once
#include "ast.hpp"
#include "liblexer/lexer.hpp"
#include "libparser/parser.hpp"
#include "libinterpreter/interpreter.hpp"
#include "libanalyzer/analyzer.hpp"
#include "token.hpp"
#include <memory>
#include <vector>

namespace BlsLang {

    class Compiler {
        public:
            void compileFile(const std::string& source);
            void compileSource(const std::string& source);
            auto& getOblocks() { return oblocks; }
            auto& getDeviceDescriptors() { return analyzer.getDeviceDescriptors(); }
            auto& getOblockDescriptors() { return analyzer.getOblockDescriptors(); }
        
        private:
            std::vector<Token> tokens;
            std::unique_ptr<AstNode> ast;
            Lexer lexer;
            Parser parser;
            Analyzer analyzer;
            Interpreter masterInterpreter;
            std::vector<Interpreter> euInterpreters;
            std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks;
    };

}