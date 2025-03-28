#pragma once
#include "boost/range/iterator_range_core.hpp"
#include "include/Common.hpp"
#include "call_stack.hpp"
#include "libtypes/bls_types.hpp"
#include "visitor.hpp"
#include "include/reserved_tokens.hpp"
#include <cstdint>
#include <iostream>
#include <any>
#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace BlsLang {
    
    class Interpreter : public Visitor {
        public:
            Interpreter() = default;

            #define AST_NODE_ABSTRACT(...)
            #define AST_NODE(Node) \
            std::any visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE

            auto& getOblocks() { return oblocks; }
            auto& getDeviceDescriptors() { return deviceDescriptors; }
            auto& getOblockDescriptors() { return oblockDescriptors; }

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

            BlsType& resolve(std::any& val);

            CallStack<std::string> cs;
            std::unordered_map<std::string, std::function<std::any(Interpreter&, std::vector<BlsType>)>> procedures = {
                {"println", [](Interpreter&, std::vector<BlsType> args) -> std::any {
                    for (auto&& arg : args) {
                        if (std::holds_alternative<int64_t>(arg)) {
                            std::cout << std::get<int64_t>(arg) << std::endl;
                        }
                        else if (std::holds_alternative<double>(arg)) {
                            std::cout << std::get<double>(arg) << std::endl;
                        }
                        else if (std::holds_alternative<bool>(arg)) {
                            std::cout << ((std::get<bool>(arg)) ? "true" : "false") << std::endl;
                        }
                        else if (std::holds_alternative<std::string>(arg)) {
                            std::cout << std::get<std::string>(arg) << std::endl;
                        }
                    }
                    return std::monostate();
                }},
                {"print", [](Interpreter&, std::vector<BlsType> args) -> std::any {
                    if (args.size() > 0) {
                        if (std::holds_alternative<int64_t>(args.at(0))) {
                            std::cout << std::get<int64_t>(args.at(0)) << std::flush;
                        }
                        else if (std::holds_alternative<double>(args.at(0))) {
                            std::cout << std::get<double>(args.at(0)) << std::flush;
                        }
                        else if (std::holds_alternative<bool>(args.at(0))) {
                            std::cout << ((std::get<bool>(args.at(0))) ? "true" : "false") << std::flush;
                        }
                        else if (std::holds_alternative<std::string>(args.at(0))) {
                            std::cout << std::get<std::string>(args.at(0)) << std::flush;
                        }
                    }
                    for (auto&& arg : boost::make_iterator_range(args.begin()+1, args.end())) {
                        if (std::holds_alternative<int64_t>(arg)) {
                            std::cout << " " << std::get<int64_t>(arg) << std::flush;
                        }
                        else if (std::holds_alternative<double>(arg)) {
                            std::cout << " " << std::get<double>(arg) << std::flush;
                        }
                        else if (std::holds_alternative<bool>(arg)) {
                            std::cout << " " << ((std::get<bool>(arg)) ? "true" : "false") << std::flush;
                        }
                        else if (std::holds_alternative<std::string>(arg)) {
                            std::cout << " " << std::get<std::string>(arg) << std::flush;
                        }
                    }
                    std::cout << std::endl;
                    return std::monostate();
                }}
            };

        std::unordered_map<std::string, std::function<std::vector<BlsType>(Interpreter&, std::vector<BlsType>)>> oblocks;
        std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
        std::vector<OBlockDesc> oblockDescriptors;

    };

}