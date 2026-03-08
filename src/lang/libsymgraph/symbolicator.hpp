#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <string>
#include <unordered_set>

namespace BlsLang {

    class Symbolicator : public Visitor {
        public:
            Symbolicator() = default;

            BlsObject visit(AstNode::Expression::Access& ast) override;
            BlsObject visit(AstNode::Statement::Declaration& ast) override;
        
            auto& getSymbols() { return symbols; }

        private:
            std::unordered_set<std::string> symbols;
    };

}