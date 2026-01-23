#pragma once
#include "ast.hpp"
#include "include/reserved_tokens.hpp"
#include "bls_types.hpp"
#include <functional>
#include <variant>

namespace BlsLang {

    class Visitor {
        public:
            #define AST_NODE(Node, ...) \
            virtual BlsObject visit(Node& ast) = 0;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE

            virtual ~Visitor() = default;
        
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

            constexpr BINARY_OPERATOR getBinOpEnum(std::string_view op) {
                if (op == LOGICAL_AND) return BINARY_OPERATOR::AND;
                if (op == LOGICAL_OR) return BINARY_OPERATOR::OR;
                if (op == COMPARISON_LT) return BINARY_OPERATOR::LT;
                if (op == COMPARISON_LE) return BINARY_OPERATOR::LE;
                if (op == COMPARISON_GT) return BINARY_OPERATOR::GT;
                if (op == COMPARISON_GE) return BINARY_OPERATOR::GE;
                if (op == COMPARISON_NE) return BINARY_OPERATOR::NE;
                if (op == COMPARISON_EQ) return BINARY_OPERATOR::EQ;
                if (op == ARITHMETIC_ADDITION) return BINARY_OPERATOR::ADD;
                if (op == ARITHMETIC_SUBTRACTION) return BINARY_OPERATOR::SUB;
                if (op == ARITHMETIC_MULTIPLICATION) return BINARY_OPERATOR::MUL;
                if (op == ARITHMETIC_DIVISION) return BINARY_OPERATOR::DIV;
                if (op == ARITHMETIC_REMAINDER) return BINARY_OPERATOR::MOD;
                if (op == ARITHMETIC_EXPONENTIATION) return BINARY_OPERATOR::EXP;
                if (op == ASSIGNMENT) return BINARY_OPERATOR::ASSIGN;
                if (op == ASSIGNMENT_ADDITION) return BINARY_OPERATOR::ASSIGN_ADD;
                if (op == ASSIGNMENT_SUBTRACTION) return BINARY_OPERATOR::ASSIGN_SUB;
                if (op == ASSIGNMENT_MULTIPLICATION) return BINARY_OPERATOR::ASSIGN_MUL;
                if (op == ASSIGNMENT_DIVISION) return BINARY_OPERATOR::ASSIGN_DIV;
                if (op == ASSIGNMENT_REMAINDER) return BINARY_OPERATOR::ASSIGN_MOD;
                if (op == ASSIGNMENT_EXPONENTIATION) return BINARY_OPERATOR::ASSIGN_EXP;
                return BINARY_OPERATOR::COUNT;
            }

            constexpr UNARY_OPERATOR getUnOpEnum(std::string_view op) {
                if (op == UNARY_NOT) return UNARY_OPERATOR::NOT;
                if (op == UNARY_NEGATIVE) return UNARY_OPERATOR::NEG;
                if (op == UNARY_INCREMENT) return UNARY_OPERATOR::INC;
                if (op == UNARY_DECREMENT) return UNARY_OPERATOR::DEC;
                return UNARY_OPERATOR::COUNT;
            }

            BlsType resolve(BlsObject&& obj);
            BlsType& resolve(BlsObject& obj);
    };

    inline BlsType Visitor::resolve(BlsObject&& obj) {
        if (std::holds_alternative<BlsType>(obj)) {
            return std::get<BlsType>(obj);
        }
        else {
            return std::get<std::reference_wrapper<BlsType>>(obj);
        }
    }

    inline BlsType& Visitor::resolve(BlsObject& obj) {
        if (std::holds_alternative<BlsType>(obj)) {
            return std::ref(std::get<BlsType>(obj));
        }
        else {
            return std::get<std::reference_wrapper<BlsType>>(obj);
        }
    }

}