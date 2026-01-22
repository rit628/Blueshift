#pragma once
#include "ast.hpp"
#include "Serialization.hpp"
#include "symgraph.hpp"
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>


namespace BlsLang{

    // Controller Source 
    struct ControllerSource{
        std::unique_ptr<AstNode::Source> ctlSource; 
        std::unordered_map<TaskID, TaskDescriptor> taskDesc; 
        
        std::vector<std::unique_ptr<AstNode::Statement>> setup_statements; 
        std::vector<std::unique_ptr<AstNode::Function>> task_list;
        
    }; 


    struct TaskCopyInfo{
        std::stack<std::vector<std::unique_ptr<AstNode::Statement>>> blockStack; 
        std::unordered_map<SymbolID,  bool> symbolDeclMap; 
        std::unique_ptr<AstNode::Function::Task> taskPtr;

        TaskCopyInfo() = default; 

        TaskCopyInfo(const TaskCopyInfo&) = delete;
        TaskCopyInfo& operator=(const TaskCopyInfo&) = delete;

        TaskCopyInfo(TaskCopyInfo&&) = default;
        TaskCopyInfo& operator=(TaskCopyInfo&&) = default;

    }; 


    class Divider : public Printer{

    private: 
        // What is to be returned by the divider
        std::unordered_map<ControllerID, ControllerSource> ctlSourceMap;  

        /*
        * Divider Metadata (Produced by Symbolic Dependency Graph)
        */

        DividerMetadata DivMeta; 

        /*
            Divider Context
        */
        std::unordered_map<TaskID, TaskCopyInfo> taskCopyMap; 
        bool inTask = true; 
        TaskID currTask; 
        bool inSetup = false; 
        // Used when visiting an expression node (with no access to splits)
        TaskID subTaskName; 
        
        
    public: 
        Divider():Printer(std::cout) {}

        void setMetadata(DividerMetadata& newData){
            this->DivMeta = newData; 
        }

        BlsObject visit(AstNode::Source& ast) override; 
        BlsObject visit(AstNode::Setup &ast) override; 
        BlsObject visit(AstNode::Function::Task& ast) override; 
        BlsObject visit(AstNode::Statement::Declaration& ast) override;
        BlsObject visit(AstNode::Statement::Expression& ast) override; 
        BlsObject visit(AstNode::Statement::If &ast) override; 

        // Final Product
        std::unordered_map<ControllerID, ControllerSource>& getControllerSplit(){
            return this->ctlSourceMap; 
        } 

    }; 

}
