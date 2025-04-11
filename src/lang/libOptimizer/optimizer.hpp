#include "ast.hpp"
#include "boost/range/iterator_range_core.hpp"
#include "include/Common.hpp"
#include "print_visitor.hpp"
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
#include <unordered_map>

using DeviceID = int; 
using OblockID = int; 

using namespace BlsLang;

struct ASTDeviceDesc {
    std::string name; 
    std::string ctl; 
    // Vectors of binded oblocks
    std::vector<OblockID> writeBindOblocks; 
    std::vector<OblockID> readBindOblocks; 
}; 

struct ASTOblockDesc {
    std::string name; 

    // Vectors of binded devices 
    std::vector<DeviceID> readBindDevices; 
    std::vector<DeviceID> writeBindDevices; 
}; 

class Optimizer : public Printer {
    private: 
        std::unordered_map<OblockID, ASTOblockDesc> oblockMap;
        std::unordered_map<DeviceID, ASTDeviceDesc> deviceMap; 

    public: 
        Optimizer() = default; 
        #define AST_NODE_ABSTRACT(...)
        #define AST_NODE(Node) \
        std::any visit(Node& ast) override;
        #include "include/NODE_TYPES.LIST"
        #undef AST_NODE_ABSTRACT
        #undef AST_NODE



};  