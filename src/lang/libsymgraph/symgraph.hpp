#pragma once

#include "../libdepgraph/depgraph.hpp"
#include "ast.hpp"
#include "include/Common.hpp"
#include "symbolicator.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <deque>
#include <stack> 
#include <vector> 

// Contains a mapping from each statement to the device is can be split to. 
using StatementPtr = uint32_t; 


namespace BlsLang{

    struct TaskData{
        std::vector<std::string> parameterList; 
        TaskDescriptor taskDesc; 
    }; 

    struct ControllerMetadata{
        // Task List
        ControllerID ctlName; 
        std::unordered_map<TaskID, TaskData> taskData; 
    }; 

    // Data from dep graph needed to function (in addition to the annotation)
    struct DividerMetadata{
        std::unordered_map<ControllerID, ControllerMetadata> ctlMetaData; 
        std::unordered_map<TaskID ,std::unordered_set<ControllerID>> TaskControllerSplit; 
    }; 


    enum class StatementType{
        EXPR, 
        TASK,
        IF, 
        ELSE,
        FOR, 
        WHILE, 
        FUNCTION, 
        STATEMENT, 
    }; 

    struct StatementData{
        AstNode::Statement* ptr; 
        std::shared_ptr<StatementData> initiatorPtr = nullptr; 
        StatementType stype; 
        std::unordered_set<DeviceID> devDeps;
        std::unordered_set<SymbolID> symbolDeps;  
    }; 

    struct SymbolData{
        SymbolID symbolName; 
        std::unordered_set<SymbolID> symbolDeps; 
        std::unordered_set<DeviceID> symbolDevice; 
        std::shared_ptr<StatementData> statementInit; 
    }; 

    struct DeviceRedef{
        SymbolID symbol; 
        SymbolID object; 
        std::string attr;
        std::shared_ptr<StatementData> redefStatement; 
    }; 

    struct DeviceRedefsCtx{
        std::deque<DeviceRedef> deviceRedefStack; 
    }; 

    // Current Context
    struct divContext{
        // Current task
        TaskID currentTask;  

        // Contains data about the task context
        std::unordered_map<SymbolID, DeviceID> aliasDevMap; 
        std::stack<std::shared_ptr<StatementData>> parentData; 
        std::unordered_map<SymbolID, SymbolData> symbolMap; 
        std::unordered_map<SymbolID, int> redefCount;
         
        // Determines if the current node is a left hand assignment (special at access)
        bool leftHandAs = false; 

        // Device RedefContext
        std::unordered_map<DeviceID, DeviceRedefsCtx> devRedefMap; 

        // Group all relevant symbols for every attribut specific to a device (to divide out the device) 
        std::unordered_map<DeviceID, std::unordered_set<SymbolID>> deviceAssignmentSymbols; 
    }; 

    class Symgraph : public Printer{
        private: 
            // End Product of the symbolic dep graph (other than AST statement annotation)
            DividerMetadata finalMetadata; 
            
            // Multi data
            std::unordered_map<TaskID, TaskContext> taskCtxMap; 
            divContext ctx; 
            //std::unordered_map<TaskID, divContext> divContextMap; 
            //std::unordered_map<DeviceID, DeviceDescriptor> deviceDescriptors; 
            std::unordered_map<TaskID, TaskDescriptor> taskDescriptorMap; 
            Printer p; 

            std::shared_ptr<StatementData> makeStData(AstNode::Statement* ptr, StatementType stype);
            std::pair<std::unordered_set<SymbolID>, std::unordered_set<DeviceID>> getDeps(AstNode& ast);
            
        
            static std::unordered_set<SymbolID> symbolicate(AstNode& ast) {
                Symbolicator s;
                ast.accept(s);
                return s.getSymbols();
            }

            void clearContext(); 
    
        public: 
            Symgraph() : 
            Printer(std::cout), p(std::cout){}

           ~Symgraph() = default; 
            
            BlsObject visit(AstNode::Source& ast) override; 
            BlsObject visit(AstNode::Function::Task& ast) override; 
            BlsObject visit(AstNode::Statement::Declaration& ast) override;
            BlsObject visit(AstNode::Statement::Expression& ast) override; 
            BlsObject visit(AstNode::Statement::If &ast) override; 
            
            BlsObject visit(AstNode::Expression::Access& ast) override; 
            BlsObject visit(AstNode::Expression::Binary &ast) override; 
       
            /*
                Debug and Drill Function Prototypes
            */
            void setMetadata(std::unordered_map<TaskID, TaskContext> taskCtxMap, std::vector<TaskDescriptor> &centralizedTasks){
                this->taskCtxMap = taskCtxMap; 
                for(auto& taskDesc : centralizedTasks){
                    this->taskDescriptorMap[taskDesc.name] = taskDesc; 
                }
            }
   
            /* 
                Performing The Divisions
            */

            std::vector<std::pair<DeviceID, ControllerID>> findDivisions(); 
          
            // Gets dependendency list in order
            void exhaustiveSymbolFillH(const SymbolID &originSymbol, std::unordered_set<SymbolID>& visitedSymbols, ControllerID &ctl, std::unordered_set<AstNode::Statement*> &visitedStatements); 
            void exhaustiveStatementFillH(std::shared_ptr<StatementData> st, std::unordered_set<SymbolID>& visitedSymbols, ControllerID &ctl, std::unordered_set<AstNode::Statement*> &visitedStatements);
            void annotateControllerDivide(); 
            DividerMetadata getDivisionData(); 
            
         
    }; 
} 