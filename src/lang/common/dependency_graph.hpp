#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "symbolicator.hpp"
#include "visitor.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace BlsLang {
    class DependencyGraph : public Visitor {
        public:
            DependencyGraph() = default;
            
            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
        
            struct DependencyData {
                std::unordered_map<std::string, DeviceDescriptor> deviceDependencies;
                std::unordered_set<std::string> symbolDependencies;
                std::vector<AstNode*> statementDependencies;
            };

            auto& getOblockStatementDependencies() { return oblockStatementDependencies; }
            auto& getOblockSymbolDependencies() { return oblockSymbolDependencies; }

        private:
            using global_statement_dependency_t = std::unordered_map<AstNode*, DependencyData>;
            using global_symbol_dependency_t = std::unordered_map<std::string, DependencyData>;

            std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
            std::unordered_map<std::string, global_statement_dependency_t> oblockStatementDependencies;
            std::unordered_map<std::string, global_symbol_dependency_t> oblockSymbolDependencies;
            global_statement_dependency_t* currentStatementDependencies;
            global_symbol_dependency_t* currentSymbolDependencies;

            static std::unordered_set<std::string> symbolicate(AstNode& ast) {
                Symbolicator s;
                ast.accept(s);
                return s.getSymbols();
            }
    };
}
