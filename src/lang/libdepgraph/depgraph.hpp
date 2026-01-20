
#pragma once
#include "ast.hpp"
#include "print_visitor.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

using SymbolID = std::string; 
using TaskID = std::string; 
using DeviceID = std::string;  
using ControllerID = std::string; 

struct AstTaskDesc{
    TaskID name; 

    std::unordered_set<DeviceID> inDeviceList;
    std::unordered_set<DeviceID> outDeviceList;  

    std::unordered_map<SymbolID, DeviceID> deviceAliasMap; 
    std::vector<DeviceID> bindedDevices; 
}; 

struct AstDeviceDesc{
    DeviceID name;
    ControllerID ctl_name;  

    std::unordered_set<TaskID> inTaskList; 
    std::unordered_set<TaskID> outTaskList; 
}; 

/*
Dep Graph Context
*/

struct TaskContext{
    TaskID operatingTask; 
    std::unordered_map<SymbolID, DeviceID> devAliasMap; 
    std::unordered_set<DeviceID> tempDevices; 
    bool isReading = true;
    bool isRW = false; 
}; 

struct SetupContext{
    bool inSetup = false; 

}; 

struct GlobalContext{
    std::unordered_map<TaskID, AstTaskDesc> taskConnections; 
    std::unordered_map<DeviceID, AstDeviceDesc> deviceConnections; 
}; 


namespace BlsLang{

    class DepGraph : public Printer{

        private: 
            TaskContext taskCtx; 
            SetupContext setupCtx; 
            GlobalContext globalCtx; 

            // Task List: 
            std::vector<TaskContext> taskCtxList;

            //utility functions: 
            void clearTaskCtx(); 
            bool isDevice(const SymbolID &candidate); 

        public: 

        DepGraph() : Printer(std::cout){}; 

            std::unordered_map<TaskID, TaskContext> getTaskMap(){
                std::unordered_map<TaskID, TaskContext> sysCtx; 
                for(auto& task : this->taskCtxList){
                    sysCtx[task.operatingTask] = task;  
                }
                return sysCtx; 
            }

        
            /*
            #define AST_NODE_ABSTRACT(...)
                #define AST_NODE(Node) \
                BlsObject visit(Node& ast) override; 
                #include "include/NODE_TYPES.LIST"
                #undef AST_NODE_ABSTRACT
                #undef AST_NODE
            */ 

            GlobalContext& getGlobalContext();
            TaskContext& getTaskContext(){return this->taskCtx;}
            // Debug Helpers: 
            void printGlobalContext(); 

            BlsObject visit(AstNode::Source& ast) override; 
            BlsObject visit(AstNode::Setup& ast) override; 

            BlsObject visit(AstNode::Statement::Declaration& ast) override; 
            BlsObject visit(AstNode::Expression::Function& ast) override; 
            BlsObject visit(AstNode::Expression::Access& ast) override; 
            BlsObject visit(AstNode::Expression::Literal& ast) override; 
            BlsObject visit(AstNode::Statement::Expression& ast) override; 
            BlsObject visit(AstNode::Function::Task &ast) override; 
            BlsObject visit(AstNode::Expression::Binary &ast) override; 
            BlsObject visit(AstNode::Statement::If& ast) override; 
            BlsObject visit(AstNode::Statement::For& ast) override; 
            BlsObject visit(AstNode::Statement::While& ast) override; 
            BlsObject visit(AstNode::Expression::Group& ast) override; 
            BlsObject visit(AstNode::Expression::Unary& ast) override;



   

    

    
    }; 

    

}

