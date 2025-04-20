#pragma once
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
            std::any visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE_ABSTRACT
            #undef AST_NODE
            
            auto& getDeviceDescriptors() { return deviceDescriptors; }
            auto& getOblockDescriptors() { return oblockDescriptors; }

        private:
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
            std::unordered_map<std::string, DeviceDescriptor> deviceDescriptors;
            std::vector<OBlockDesc> oblockDescriptors;
            std::unordered_map<BlsType, uint8_t> literalPool = {
                {std::monostate(), 0},  // for void return values
                {1, 1}  // for increment & decrement expressions
            };
            uint8_t literalCount = literalPool.size();
    };

}