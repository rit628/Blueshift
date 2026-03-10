#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <utility>

namespace BlsLang {

    class Printer : public Visitor {
        private:
            BlsObject printChild(auto& value);
            template<std::derived_from<AstNode> Node>
            BlsObject printChild(std::unique_ptr<Node>& node);
            template<std::derived_from<AstNode> Node>
            BlsObject printChild(std::optional<std::unique_ptr<Node>>& node);
            template<std::derived_from<AstNode> Node>
            BlsObject printChild(std::vector<std::unique_ptr<Node>>& nodes);
            template<std::derived_from<AstNode> Node>
            BlsObject printChild(std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& nodes);
            template<typename T>
            BlsObject printChild(std::pair<const char *&&, T&>& value);

            template<std::derived_from<AstNode> Node>
            BlsObject printChildren(Node& node);

        public:
            Printer(std::ostream& os) : os(os) {}
            #define AST_NODE(Node, ...) \
            inline BlsObject visit(Node& ast) override { \
                os << indent + #Node " {\n"; \
                indent.append(4, ' '); \
                auto result = printChildren(ast); \
                indent.resize(indent.size() - 4); \
                os << "\n" + indent + "}" << std::endl; \
                return result; \
            }
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE

        private:
            std::ostream& os;
            std::string indent = "";
    };

}

#include "printer.tpp"