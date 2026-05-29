#pragma once
#include "ast.hpp"
#include "Serialization.hpp"
#include "generator.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include "analyzer.hpp"
#include "depgraph.hpp"
#include "symgraph.hpp"
#include "divider.hpp"
#include "token.hpp"
#include <memory>
#include <variant>
#include <vector>

namespace BlsLang {

    class Compiler {
        public:
            Compiler()
            : generator(analyzer.getBoundTasks()
                      , analyzer.getBoundTaskMap()
                      , analyzer.getLiteralPool()
                      , analyzer.getFunctionSymbols()) {}
            
            using ostream_t = std::variant<std::reference_wrapper<std::vector<char>>, std::reference_wrapper<std::ostream>>;

            void compileFile(const std::string& source, ostream_t outputStream = std::cout);
            void compileSource(const std::string& source, ostream_t outputStream = std::cout);
            auto& getAst() { return ast; }
            auto& getTaskDescriptors() { return analyzer.getBoundTasks(); }
            auto& getTaskDescriptorMap() { return analyzer.getBoundTaskMap(); }
            auto getTaskContexts(){return depGraph.getTaskMap();}
            auto getGlobalContext() {return depGraph.getGlobalContext();}
            
        private:
            std::vector<Token> tokens;
            std::unique_ptr<AstNode> ast;
            Lexer lexer;
            Parser parser;
            Analyzer analyzer;
            DepGraph depGraph;
            Generator generator;
            Symgraph symGraph; 
            Divider divider; 
    };

}