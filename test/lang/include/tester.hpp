#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <gtest/gtest.h>
#include <stack>

namespace BlsLang {
    class Tester : public Visitor {
        private:
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
            Tester(std::unique_ptr<AstNode> expectedAst) : expectedAst(std::move(expectedAst)) {}
            void addExpectedAst(std::unique_ptr<AstNode> expectedAst) { this->expectedAst = std::move(expectedAst); }
            #define AST_NODE(Node, ...) \
            inline BlsObject visit(Node& ast) override { \
                auto& toCast = (expectedVisits.empty()) ? expectedAst : expectedVisits.top(); \
                auto& expected = dynamic_cast<Node&>(*toCast); \
                return testChildren(ast, expected); \
            }
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
    
            std::unique_ptr<AstNode> expectedAst;
            std::stack<std::unique_ptr<AstNode>> expectedVisits;
    };
}

#include "tester.tpp"