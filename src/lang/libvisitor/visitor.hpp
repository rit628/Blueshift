#pragma once
#include "ast.hpp"
#include "reserved_tokens.hpp"
#include "bls_types.hpp"
#include <concepts>
#include <memory>

namespace BlsLang {

    class VisitorBase {
        public:
            #define AST_NODE(Node, ...) \
            virtual BlsObject visit(Node& ast) = 0;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE

            virtual void preVisit(AstNode&) = 0;
            virtual void postVisit(AstNode&) = 0;

            virtual ~VisitorBase() = default;
    };

    class Visitor : public VisitorBase {
        private:
            BlsObject visitChild(auto&);
            template<std::derived_from<AstNode> Node>
            BlsObject visitChild(std::unique_ptr<Node>& node);
            template<std::derived_from<AstNode> Node>
            BlsObject visitChild(std::optional<std::unique_ptr<Node>>& node);
            template<std::derived_from<AstNode> Node>
            BlsObject visitChild(std::vector<std::unique_ptr<Node>>& nodes);
            template<std::derived_from<AstNode> Node>
            BlsObject visitChild(std::vector<std::pair<std::unique_ptr<Node>, std::unique_ptr<Node>>>& nodes);
        
        public:
            template<std::derived_from<AstNode> Node>
            BlsObject visitChildren(Node& node);

            #define AST_NODE(Node, ...) \
            inline virtual BlsObject visit(Node& ast) override { return visitChildren(ast); }
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE

            inline virtual void preVisit(AstNode&) override { }
            inline virtual void postVisit(AstNode&) override { }

        protected:
            enum class BINARY_OPERATOR : uint8_t {
                  AND
                , OR
                , LT
                , LE
                , GT
                , GE
                , EQ
                , NE
                , ADD
                , SUB
                , MUL
                , DIV
                , MOD
                , EXP
                , ASSIGN
                , ASSIGN_ADD
                , ASSIGN_SUB
                , ASSIGN_MUL
                , ASSIGN_DIV
                , ASSIGN_MOD
                , ASSIGN_EXP
                , COUNT
            };

            enum class UNARY_OPERATOR : uint8_t {
                  NOT
                , NEG
                , INC
                , DEC
                , COUNT
            };

            static constexpr BINARY_OPERATOR getBinOpEnum(std::string_view op);
            static constexpr UNARY_OPERATOR getUnOpEnum(std::string_view op);

            static BlsType resolve(BlsObject&& obj);
            static BlsType& resolve(BlsObject& obj);
    };

}

#include "visitor.tpp"