#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "libgenerator/generator.hpp"
#include "dependency_graph.hpp"
#include "liblexer/lexer.hpp"
#include "libparser/parser.hpp"
#include "libinterpreter/interpreter.hpp"
#include "libanalyzer/analyzer.hpp"
#include "libdepgraph/depgraph.hpp"
#include "libsymgraph/symgraph.hpp"
#include "libdivider/divider.hpp"
#include "token.hpp"
#include <memory>
#include <vector>

namespace BlsLang {

    class Compiler {
        public:
            Compiler()
            : generator(analyzer.getOblockDescriptors()
                       , analyzer.getLiteralPool()) {}
            
            void compileFile(const std::string& source, std::ostream& outputStream = std::cout);
            void compileSource(const std::string& source, std::ostream& outputStream = std::cout);
            auto& getOblocks() { return oblocks; }
            auto& getOblockDescriptors() { return oblockDescriptors; }
            auto& getOblockDescriptorMap() { return analyzer.getOblockDescriptors(); }
            auto getOblockContexts(){return depGraph.getOblockMap();}
            auto getGlobalContext() {return depGraph.getGlobalContext();}
            
        private:
            std::vector<Token> tokens;
            std::unique_ptr<AstNode> ast;
            Lexer lexer;
            Parser parser;
            Analyzer analyzer;
            DepGraph depGraph;
            Generator generator;
            Interpreter masterInterpreter;
            Symgraph symGraph; 
            Divider divider; 
            std::vector<Interpreter> euInterpreters;
            std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks;
            std::vector<OBlockDesc> oblockDescriptors;
    };

}