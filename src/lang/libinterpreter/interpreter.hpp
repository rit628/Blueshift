#pragma once
#include "ast.hpp"
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
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE

            auto& getOblocks() { return oblocks; }

        private:
            CallStack<std::string> cs;
            std::unordered_map<std::string, std::function<BlsType(Interpreter&, std::vector<BlsType>)>> procedures = {
                {"println", [](Interpreter&, std::vector<BlsType> args) {
                    for (auto&& arg : args) {
                        std::cout << arg << std::endl;
                    }
                    return std::monostate();
                }},
                {"print", [](Interpreter&, std::vector<BlsType> args) {
                    if (args.size() > 0) {
                        std::cout << args.at(0) << std::flush;
                        for (auto&& arg : boost::make_iterator_range(args.begin()+1, args.end())) {
                            std::cout << " " << arg << std::flush;
                        }
                        std::cout << std::endl;
                    }
                    return std::monostate();
                }}
            };

        std::unordered_map<std::string, std::function<std::vector<BlsType>(Interpreter&, std::vector<BlsType>)>> oblocks;

    };

}