#pragma once
#include "call_stack.hpp"
#include "bls_types.hpp"
#include "visitor.hpp"
#include "include/reserved_tokens.hpp"
#include <any>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace BlsLang {
    
    class Interpreter : public Visitor {
        public:
            enum class OPERATOR : uint8_t {
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
              , COUNT
            };

            constexpr OPERATOR getOpEnum(std::string_view op) {
                if (op == LOGICAL_AND) return OPERATOR::AND;
                if (op == LOGICAL_OR) return OPERATOR::OR;
                if (op == COMPARISON_LT) return OPERATOR::LT;
                if (op == COMPARISON_LE) return OPERATOR::LE;
                if (op == COMPARISON_GT) return OPERATOR::GT;
                if (op == COMPARISON_GE) return OPERATOR::GE;
                if (op == COMPARISON_NE) return OPERATOR::NE;
                if (op == COMPARISON_EQ) return OPERATOR::EQ;
                if (op == ARITHMETIC_ADDITION) return OPERATOR::ADD;
                if (op == ARITHMETIC_SUBTRACTION) return OPERATOR::SUB;
                if (op == ARITHMETIC_MULTIPLICATION) return OPERATOR::MUL;
                if (op == ARITHMETIC_DIVISION) return OPERATOR::DIV;
                if (op == ARITHMETIC_REMAINDER) return OPERATOR::MOD;
                if (op == ARITHMETIC_EXPONENTIATION) return OPERATOR::EXP;
                return OPERATOR::COUNT;
            }

            Interpreter() = default;

            #define AST_NODE_ABSTRACT(_)
            #define AST_NODE(Node) \
            std::any visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE

            BlsType& resolve(std::any& val);

        private:
            CallStack<std::string> cs;
            std::unordered_map<std::string, std::function<std::any(std::vector<BlsType>)>> functions;
    };

}