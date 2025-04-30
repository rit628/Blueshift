#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <any>

namespace BlsLang {

    class Printer : public Visitor {
        public:
            Printer(std::ostream& os) : os(os) {}
            #define AST_NODE_ABSTRACT(...)
            #define AST_NODE(Node) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE

        private:
            std::ostream& os;
    };

}