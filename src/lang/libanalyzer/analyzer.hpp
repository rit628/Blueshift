#pragma once
#include "libtypes/bls_types.hpp"
#include "call_stack.hpp"
#include "include/reserved_tokens.hpp"
#include "visitor.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace BlsLang {

    class Analyzer : public Visitor {
        public:
            Analyzer() = default;

            #define AST_NODE_ABSTRACT(...)
            #define AST_NODE(Node) \
            std::any visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE
            
        private:
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

            constexpr BINARY_OPERATOR getBinOpEnum(std::string_view op);
            constexpr UNARY_OPERATOR getUnOpEnum(std::string_view op);
            BlsType& resolve(std::any& val);

            struct FunctionSignature {
                std::string name;
                BlsType returnType;
                std::vector<BlsType> parameterTypes;
                bool variadic = false;
            };

            CallStack<std::string> cs;
            std::unordered_map<std::string, FunctionSignature> procedures = {
                {"println", {"println", std::monostate(), {}, true}},
                {"print", {"print", std::monostate(), {}, true}}
            };
            std::unordered_map<std::string, FunctionSignature> oblocks;
            std::unordered_set<BlsType> literals;
    };

    inline constexpr Analyzer::BINARY_OPERATOR Analyzer::getBinOpEnum(std::string_view op) {
        if (op == LOGICAL_AND) return Analyzer::BINARY_OPERATOR::AND;
        if (op == LOGICAL_OR) return Analyzer::BINARY_OPERATOR::OR;
        if (op == COMPARISON_LT) return Analyzer::BINARY_OPERATOR::LT;
        if (op == COMPARISON_LE) return Analyzer::BINARY_OPERATOR::LE;
        if (op == COMPARISON_GT) return Analyzer::BINARY_OPERATOR::GT;
        if (op == COMPARISON_GE) return Analyzer::BINARY_OPERATOR::GE;
        if (op == COMPARISON_NE) return Analyzer::BINARY_OPERATOR::NE;
        if (op == COMPARISON_EQ) return Analyzer::BINARY_OPERATOR::EQ;
        if (op == ARITHMETIC_ADDITION) return Analyzer::BINARY_OPERATOR::ADD;
        if (op == ARITHMETIC_SUBTRACTION) return Analyzer::BINARY_OPERATOR::SUB;
        if (op == ARITHMETIC_MULTIPLICATION) return Analyzer::BINARY_OPERATOR::MUL;
        if (op == ARITHMETIC_DIVISION) return Analyzer::BINARY_OPERATOR::DIV;
        if (op == ARITHMETIC_REMAINDER) return Analyzer::BINARY_OPERATOR::MOD;
        if (op == ARITHMETIC_EXPONENTIATION) return Analyzer::BINARY_OPERATOR::EXP;
        if (op == ASSIGNMENT) return Analyzer::BINARY_OPERATOR::ASSIGN;
        if (op == ASSIGNMENT_ADDITION) return Analyzer::BINARY_OPERATOR::ASSIGN_ADD;
        if (op == ASSIGNMENT_SUBTRACTION) return Analyzer::BINARY_OPERATOR::ASSIGN_SUB;
        if (op == ASSIGNMENT_MULTIPLICATION) return Analyzer::BINARY_OPERATOR::ASSIGN_MUL;
        if (op == ASSIGNMENT_DIVISION) return Analyzer::BINARY_OPERATOR::ASSIGN_DIV;
        if (op == ASSIGNMENT_REMAINDER) return Analyzer::BINARY_OPERATOR::ASSIGN_MOD;
        if (op == ASSIGNMENT_EXPONENTIATION) return Analyzer::BINARY_OPERATOR::ASSIGN_EXP;
        return Analyzer::BINARY_OPERATOR::COUNT;
    }

    inline constexpr Analyzer::UNARY_OPERATOR Analyzer::getUnOpEnum(std::string_view op) {
        if (op == UNARY_NOT) return Analyzer::UNARY_OPERATOR::NOT;
        if (op == UNARY_NEGATIVE) return Analyzer::UNARY_OPERATOR::NEG;
        if (op == UNARY_INCREMENT) return Analyzer::UNARY_OPERATOR::INC;
        if (op == UNARY_DECREMENT) return Analyzer::UNARY_OPERATOR::DEC;
        return Analyzer::UNARY_OPERATOR::COUNT;
    }

}