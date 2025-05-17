#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "libgenerator/generator.hpp"
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
            Compiler(std::ostream& stream = std::cout)
                   : generator(stream
                             , analyzer.getOblockDescriptors()
                             , analyzer.getLiteralPool()) {}
            
            void compileFile(const std::string& source);
            void compileSource(const std::string& source);
            auto& getOblocks() { return oblocks; }
            auto& getOblockDescriptors() { return oblockDescriptors; }
        
        private:
            std::vector<Token> tokens;
            std::unique_ptr<AstNode> ast;
            Lexer lexer;
            Parser parser;
            Analyzer analyzer;
            Generator generator;
            Interpreter masterInterpreter;
            std::vector<Interpreter> euInterpreters;
            std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks;
            std::vector<OBlockDesc> oblockDescriptors;
    };

}