#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "libtype/bls_types.hpp"
#include "libtrap/include/traps.hpp"
#include "call_stack.hpp"
#include "include/reserved_tokens.hpp"
#include "visitor.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace BlsLang {

    class Analyzer : public Visitor {
        public:
            friend class AnalyzerTest;
            Analyzer() = default;

            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
            
            auto& getOblockDescriptors() { return oblockDescriptors; }
            auto& getLiteralPool() { return literalPool; }

        private:
            void addToPool(BlsType literal) { if (!literalPool.contains(literal)) literalPool.emplace(literal, literalPool.size()); }

            struct FunctionSignature {
                std::string name;
                BlsType returnType;
                std::vector<BlsType> parameterTypes = {};
                std::unordered_map<std::string, uint8_t> parameterIndices = {};
                bool variadic = false;
            };

            CallStack<std::string> cs;
            std::unordered_map<std::string, FunctionSignature> procedures = {
                #define TRAP_BEGIN(trapName, ...) \
                { #trapName, \
                FunctionSignature{ .name = #trapName, \
                    .returnType = BlsTrap::Detail::trapName::returnType, \
                    .parameterTypes = BlsTrap::Detail::trapName::parameterTypes, \
                    .parameterIndices = BlsTrap::Detail::trapName::parameterIndices, \
                    .variadic = BlsTrap::Detail::trapName::variadic
                    #define VARIADIC(...)
                    #define ARGUMENT(...)
                    #define TRAP_END \
                }},
                #include "libtrap/include/TRAPS.LIST"
                #undef TRAP_BEGIN
                #undef ARGUMENT
                #undef TRAP_END
            };
            std::unordered_map<std::string, FunctionSignature> oblocks;
            std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
            std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
            std::unordered_map<BlsType, uint8_t> literalPool;
    };

}