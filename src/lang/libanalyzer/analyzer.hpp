#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "libtypes/bls_types.hpp"
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

            #define AST_NODE_ABSTRACT(...)
            #define AST_NODE(Node) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE
            
            auto& getOblockDescriptors() { return oblockDescriptors; }
            auto& getLiteralPool() { return literalPool; }

        private:
            void addToPool(BlsType literal) { if (!literalPool.contains(literal)) literalPool.emplace(literal, literalCount++); }

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
            std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
            std::unordered_map<std::string, OBlockDesc> oblockDescriptors;
            std::unordered_map<BlsType, uint8_t> literalPool;
            uint8_t literalCount = 0;
    };

}