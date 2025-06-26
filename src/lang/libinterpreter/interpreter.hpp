#pragma once
#include "ast.hpp"
#include "libtrap/include/traps.hpp"
#include "error_types.hpp"
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

            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE

            auto& getOblocks() { return oblocks; }

        private:
            CallStack<std::string> cs;
            std::unordered_map<std::string, std::function<BlsType(Interpreter&, std::vector<BlsType>)>> procedures = {
                #define TRAP_BEGIN(trapName, ...) \
                { #trapName, \
                [](Interpreter&, std::vector<BlsType> args) { \
                    using namespace BlsTrap; \
                    using argnum [[ maybe_unused ]] = Detail::trapName::ARGNUM; \
                    auto fn = exec__##trapName; \
                    constexpr auto variadic = Detail::trapName::variadic; \
                    if constexpr (!variadic) { \
                        if (argnum::COUNT >= 0 && args.size() != argnum::COUNT) { \
                            throw RuntimeError("Invalid number of arguments provided to " #trapName "."); \
                        } \
                    }
                    #define VARIADIC(...)
                    #define ARGUMENT(argName, argType...) \
                    if (!std::holds_alternative<converted_t<argType>>(args.at(argnum::argName))) { \
                        throw RuntimeError("Invalid type for argument " #argName "."); \
                    }
                    #define TRAP_END \
                    return fn(args); \
                }},
                #include "libtrap/include/TRAPS.LIST"
                #undef TRAP_BEGIN
                #undef VARIADIC
                #undef ARGUMENT
                #undef TRAP_END
            };

        std::unordered_map<std::string, std::function<std::vector<BlsType>(Interpreter&, std::vector<BlsType>)>> oblocks;

    };

}