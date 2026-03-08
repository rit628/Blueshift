#include "divider.hpp"
#include "ast.hpp"
#include <algorithm>
#include <memory>
#include <sys/stat.h>
#include <variant>

using namespace BlsLang; 

BlsObject Divider::visit(AstNode::Source &ast){
    // Open the blockStacks for the setup functions
    for(auto& pair : this->DivMeta.ctlMetaData){
        auto srcCtl = std::make_unique<AstNode::Source>(); 
        ControllerSource ControllerCarrier; 
        ControllerCarrier.ctlSource = std::move(srcCtl); 
        this->ctlSourceMap[pair.first] = std::move(ControllerCarrier); 


    }
   
    for(auto& TaskPtr : ast.tasks){
        this->inTask = true; 
        TaskPtr->accept(*this); 
        this->inTask = false; 
    }
  

    // Populate the source of each object
    for(auto& pair : this->DivMeta.ctlMetaData){
        auto& ctlData = pair.second; 
        auto& srcObj = this->ctlSourceMap[ctlData.ctlName];
        auto& srcPtr = srcObj.ctlSource; 
        srcPtr->tasks = std::move(srcObj.task_list);   

        for(auto& item : ctlData.taskData){
            srcObj.taskDesc[item.second.taskDesc.name] =  item.second.taskDesc; 
        }
    }

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Setup &ast){

    this->inSetup = true; 

    for(auto& stmt : ast.statements){
        stmt->accept(*this); 
    }

    this->inSetup = false; 

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Function::Task &ast){
    auto& ctlNames = this->DivMeta.TaskControllerSplit[ast.name]; 
    TaskID ogTaskName = ast.name; 
    
  
    for(auto& ctl: ctlNames){
        TaskCopyInfo tci;
        tci.taskPtr = std::make_unique<AstNode::Function::Task>(); 
        // Naming Schema used for now
        TaskID taskName = ogTaskName + "_" + ctl; 
        tci.taskPtr->name = taskName; 
        tci.blockStack.push({}); 

        /* 
            Fill out Task header data
        */ 

        auto& derTask = this->DivMeta.ctlMetaData[ctl].taskData[ast.name];
        std::unordered_set<DeviceID> derivedDevSet; 
        tci.taskPtr->parameters = derTask.parameterList;  
        this->taskCopyMap.emplace(taskName, std::move(tci)); 
    }


    for(auto& stmt : ast.statements ){
        stmt->accept(*this); 
    }

    for(auto& ctl : ctlNames){

        TaskID taskName = ogTaskName + "_" + ctl; 
        auto& taskPtr = this->taskCopyMap[taskName].taskPtr; 
        // Copy the statements for the divided task: 

        auto& statementStack = this->taskCopyMap[taskName].blockStack; 
        if(statementStack.size() == 1){
            taskPtr->statements = std::move(statementStack.top()); 
            statementStack.pop(); 
        }
        else{
            std::cout<<"Divider Error: Statement stack at size: "<<statementStack.size()<<" when copied into Task!"<<std::endl; 
        }
        
        this->ctlSourceMap.at(ctl).task_list.push_back(std::move(taskPtr));
        
    }


    return std::monostate(); 
}





BlsObject Divider::visit(AstNode::Statement::If &ast){
    auto ctlSet = ast.controllerSplit; 

    for(auto ctl : ctlSet){
        auto taskName = this->currTask + "_" + ctl; 
        this->taskCopyMap[taskName].blockStack.push({}); 
    }

    for(auto& stmt : ast.block){
        stmt->accept(*this); 
    }

    for(auto& ctl : ctlSet){
        // Pop and add the present controller split statement
        auto taskName = this->currTask + "_" + ctl; 
        auto& derTaskMap = this->taskCopyMap[taskName].blockStack; 
        auto newStack = std::move(derTaskMap.top()); 
        derTaskMap.pop(); 
        
        // make a new statement object
        auto derivedIf = std::make_unique<AstNode::Statement::If>(); 
        *derivedIf->condition = *ast.condition;  
        derivedIf->block = std::move(newStack); 
    }


   // Do the same for if else and else

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Statement::Declaration &ast){
    auto totalCntSplit = this->DivMeta.TaskControllerSplit[this->currTask]; 
    auto writeCtls = ast.controllerSplit;
    
    if(this->inSetup){
        // Add the statement to the declaration

        return std::monostate(); 
    }

    for(auto& ctlName : totalCntSplit){
        auto TaskName = this->currTask + "_" + ctlName;
        auto splitDecl = std::make_unique<AstNode::Statement::Declaration>();

        splitDecl->name = ast.name; 

        auto& typeObj = ast.type; 
        splitDecl->type->name = typeObj->name; 

        if(writeCtls.contains(ctlName)){
            if(ast.value.has_value()){
                *splitDecl->value->get() = *ast.value->get();
            }
        }
    
        this->taskCopyMap[TaskName].blockStack.top().push_back(std::move(splitDecl));
    }

    return std::monostate(); 
}



BlsObject Divider::visit(AstNode::Statement::Expression &ast){
    if(this->inSetup){
         // Add the statement to the declaration
        
        return std::monostate(); 
    }

    auto ctlList = ast.controllerSplit; 
    for(auto& ctlName : ctlList){
        auto TaskName = this->currTask + "_" + ctlName; 
        auto newExpr = std::make_unique<AstNode::Statement::Expression>(); 
        *newExpr->expression = *ast.expression; 

        this->taskCopyMap[TaskName].blockStack.top().push_back(std::move(newExpr)); 
    }
    return std::monostate(); 
}





 


