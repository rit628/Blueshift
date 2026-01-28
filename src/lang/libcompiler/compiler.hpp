#pragma once
#include "ast.hpp"
#include "Serialization.hpp"
#include "generator.hpp"
#include "dependency_graph.hpp"
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
            : generator(analyzer.getTaskDescriptors()
                       , analyzer.getLiteralPool()
                       , analyzer.getFunctionSymbols()) {}
            
            using ostream_t = std::variant<std::reference_wrapper<std::vector<char>>, std::reference_wrapper<std::ostream>>;

            void compileFile(const std::string& source, ostream_t outputStream = std::cout);
            void compileSource(const std::string& source, ostream_t outputStream = std::cout);
            auto& getTasks() { return tasks; }
            auto& getTaskDescriptors() { return taskDescriptors; }
            auto& getTaskDescriptorMap() { return analyzer.getTaskDescriptors(); }
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
            Interpreter masterInterpreter;
            Symgraph symGraph; 
            Divider divider; 
            std::vector<Interpreter> euInterpreters;
            std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> tasks;
            std::vector<TaskDescriptor> taskDescriptors;
    };

}