#include "ast.hpp"
#include "libtypes/bls_types.hpp"
#include "print_visitor.hpp"
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
  
using SymbolID = std::string; 
using DeviceID = std::string; 
using ControllerID = std::string; 
using OblockID = std::string; 

/*
    Dependency Graph Node Stuff: 
*/

struct SymbolInfo{
    std::string symbolName; 
    bool isConstExpr= true; 
    BlsType type; 
}; 

struct StatementInfo{
    std::shared_ptr<BlsLang::AstNode> statement;
}; 

struct SymbolicNode{
    std::unordered_map<std::string, SymbolInfo> symDepMap; 
    std::unordered_map<int, StatementInfo> statementData;

    void clear(){
        symDepMap.clear(); 
        statementData.clear(); 
    }
}; 

// Depdency Graph Contexts: 

struct OblockDependencyNode{
    // Oblock Metadata: 
    OblockID name; 
    std::vector<DeviceID> bindedDevices; 

    // Graph Stuff: 
    std::vector<DeviceID> deviceReadFrom; 
    std::vector<DeviceID> deviceReadTo; 
}; 

struct DeviceDependencyNode{
    // Device Metadata
    OblockID name; 
    ControllerID hostController; 

    // Graph Stuff: 
    std::vector<OblockID> oblockSendTo; 
    std::vector<OblockID> oblockRecvFrom; 
}; 

// Dependency graph stuff: 
struct OblockCtx{
    // Maps Device Alias to actual Devices: 
    std::unordered_map<SymbolID, DeviceID> deviceAliasMap; 
    // Redef count: 
    std::unordered_map<SymbolID, int> symbolRedefCount; 
    // Symbolic Depdency Graph: 
    std::unordered_map<SymbolID, SymbolicNode> depGraph; 
    // Processing: 
    SymbolicNode processingNode; 
    // Absolute Symbol Map
    std::unordered_map<SymbolID, SymbolInfo> symbolMap; 


    // Bools to determine read/ write mode 
    bool asslhs = false; 

}; 

struct SetupCtx{
    bool inSetup = false; 
    std::vector<DeviceID> oblockArgs; 
}; 

struct GlobalCtx{
    std::unordered_map<OblockID, OblockDependencyNode> oblockDesc; 
    std::unordered_map<DeviceID, DeviceDependencyNode> deviceDesc; 
}; 

namespace BlsLang{
    class DepGraph : public Printer{

        private:
            GlobalCtx globalCtx; 
            SetupCtx setupCtx; 
            OblockCtx oblockCtx; 

            // Utility functions for string manipulation: 
            void error(std::string msg); 
            SymbolID getLastAssignment(SymbolID &symbol); 
            SymbolID getNextAssignment(SymbolID &symbol); 
            ControllerID extractCtl(std::string devConstructor); 

            // Utility function for reseting oblock context:
            void clearOblockCtx();  
        public: 
            DepGraph(): Printer(std::cout) {};
            std::any visit(AstNode::Source &ast) override; 
            std::any visit(AstNode::Setup &ast) override; 

            std::any visit(AstNode::Function::Oblock &ast) override; 

            std::any visit(AstNode::Statement::Declaration &ast) override; 
            
            std::any visit(AstNode::Expression::Access &ast) override; 
            std::any visit(AstNode::Expression::Function &ast) override; 
            std::any visit(AstNode::Expression::Literal &ast) override; 
            std::any visit(AstNode::Expression::Binary &ast) override; 

            // Receiver Functions: 
            GlobalCtx& getGlobalCtx(); 


    }; 
};

