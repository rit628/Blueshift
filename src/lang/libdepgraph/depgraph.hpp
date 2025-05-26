#pragma once
#include "ast.hpp"
#include "print_visitor.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

using SymbolID = std::string; 
using OblockID = std::string; 
using DeviceID = std::string;  
using ControllerID = std::string; 

struct AstOblockDesc{
    OblockID name; 

    std::unordered_set<DeviceID> inDeviceList;
    std::unordered_set<DeviceID> outDeviceList;  

    std::unordered_map<SymbolID, DeviceID> deviceAliasMap; 
    std::vector<DeviceID> bindedDevices; 
}; 

struct AstDeviceDesc{
    DeviceID name;
    ControllerID ctl_name;  

    std::unordered_set<OblockID> inOblockList; 
    std::unordered_set<OblockID> outOblockList; 
}; 

/*
Dep Graph Context
*/

struct OblockContext{
    OblockID operatingOblock; 
    std::unordered_map<SymbolID, DeviceID> devAliasMap; 
    std::unordered_set<DeviceID> tempDevices; 
    bool isReading = true;
    bool isRW = false; 
}; 

struct SetupContext{
    bool inSetup = false; 

}; 

struct GlobalContext{
    std::unordered_map<OblockID, AstOblockDesc> oblockConnections; 
    std::unordered_map<DeviceID, AstDeviceDesc> deviceConnections; 
}; 


namespace BlsLang{

    class DepGraph : public Printer{

        private: 
            OblockContext oblockCtx; 
            SetupContext setupCtx; 
            GlobalContext globalCtx; 

            // Oblock List: 
            std::vector<OblockContext> oblockCtxList;

            //utility functions: 
            void clearOblockCtx(); 
            bool isDevice(const SymbolID &candidate); 

        public: 
            DepGraph() : Printer(std::cout){}; 
        
            /*
            #define AST_NODE_ABSTRACT(...)
                #define AST_NODE(Node) \
                BlsObject visit(Node& ast) override; 
                #include "include/NODE_TYPES.LIST"
                #undef AST_NODE_ABSTRACT
                #undef AST_NODE
            */ 

            GlobalContext& getGlobalContext();
            OblockContext& getOblockContext(){return this->oblockCtx;}
            // Debug Helpers: 
            void printGlobalContext(); 

            BlsObject visit(AstNode::Source& ast) override; 
            BlsObject visit(AstNode::Setup& ast) override; 

            BlsObject visit(AstNode::Statement::Declaration& ast) override; 
            BlsObject visit(AstNode::Expression::Function& ast) override; 
            BlsObject visit(AstNode::Expression::Access& ast) override; 
            BlsObject visit(AstNode::Expression::Literal& ast) override; 
            BlsObject visit(AstNode::Statement::Expression& ast) override; 
            BlsObject visit(AstNode::Function::Oblock &ast) override; 
            BlsObject visit(AstNode::Expression::Binary &ast) override; 
            BlsObject visit(AstNode::Statement::If& ast) override; 
            BlsObject visit(AstNode::Statement::For& ast) override; 
            BlsObject visit(AstNode::Statement::While& ast) override; 
            BlsObject visit(AstNode::Expression::Group& ast) override; 
            BlsObject visit(AstNode::Expression::Unary& ast) override;



   

    

    
    }; 

    

}

