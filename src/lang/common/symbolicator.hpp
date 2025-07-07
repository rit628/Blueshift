#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <string>
#include <unordered_set>

namespace BlsLang {

    class Symbolicator : public Visitor {
        public:
            Symbolicator() = default;
            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
        
            auto& getSymbols() { return symbols; }

        private:
            std::unordered_set<std::string> symbols;
    };

}