#pragma once
#include <any>

namespace BlsLang {

    class AstNode;

    class Visitor {
        public:
            virtual std::any visit(AstNode& v) = 0;
            virtual ~Visitor() = default;
    };

}