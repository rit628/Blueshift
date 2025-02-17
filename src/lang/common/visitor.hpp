#pragma once
#include "ast.hpp"
#include <any>

namespace BlsLang {

    class Visitor {
        public:
            #define AST_NODE_ABSTRACT(_)
            #define AST_NODE(Node) \
            virtual std::any visit(Node& ast) = 0;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE

            virtual ~Visitor() = default;
    };

}