
#pragma once
#include "ast.hpp"
#include "visitor.hpp"
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

    class DepGraph : public Visitor {

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

            std::unordered_map<TaskID, TaskContext> getTaskMap(){
                std::unordered_map<TaskID, TaskContext> sysCtx; 
                for(auto& task : this->taskCtxList){
                    sysCtx[task.operatingTask] = task;  
                }
                return sysCtx; 
            }

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

