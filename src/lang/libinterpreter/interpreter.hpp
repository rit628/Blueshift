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