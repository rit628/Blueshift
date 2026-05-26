#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <stack>

namespace BlsLang {
    class Tester : public Visitor {
        private:
            template<std::derived_from<AstNode> Node>
            void checkedAccept(const char *& name, std::unique_ptr<Node>& node);
        
            BlsObject testChild(const char *& name, auto& value, auto& expectedValue);
            template<std::derived_from<AstNode> Node>
            BlsObject testChild(const char *& name
                              , std::unique_ptr<Node>& node
                              , std::unique_ptr<Node>& expectedNode);
            template<std::derived_from<AstNode> Node>
            BlsObject testChild(const char *& name
                              , std::optional<std::unique_ptr<Node>>& node
                              , std::optional<std::unique_ptr<Node>>& expectedNode);
            template<std::derived_from<AstNode> Node>
            BlsObject testChild(const char *& name
                              , std::vector<std::unique_ptr<Node>>& nodes
                              , std::vector<std::unique_ptr<Node>>& expectedNodes);
            template<std::derived_from<AstNode> Node>
            BlsObject testChild(const char *& name
                              , std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& nodes
                              , std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& expectedNodes);
            template<typename T>
            BlsObject testChild(std::tuple<const char *&&, T&, T&>& subject);

            template<std::derived_from<AstNode> Node>
            BlsObject testChildren(Node& node, Node& expectedNode);

        public:
            Tester() = default;
            void compare(std::unique_ptr<AstNode>& ast, std::unique_ptr<AstNode>& expectedAst);
            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
    
            AstNode* expectedAst = nullptr;
            std::stack<AstNode*> expectedVisits;
    };
}

#include "tester.tpp"