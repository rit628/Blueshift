#include "divider.hpp"
#include "ast.hpp"
#include "symbolicator.hpp"
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
   
    for(auto& TaskPtr : ast.getTasks()){
        this->inTask = true; 
        TaskPtr->accept(*this); 
        this->inTask = false; 
    }
  

    // Populate the source of each object
    for(auto& pair : this->DivMeta.ctlMetaData){
        auto& ctlData = pair.second; 
        auto& srcObj = this->ctlSourceMap[ctlData.ctlName];
        auto& srcPtr = srcObj.ctlSource; 
        srcPtr->getTasks() = std::move(srcObj.task_list);   

        for(auto& item : ctlData.taskData){
            srcObj.taskDesc[item.second.taskDesc.name] =  item.second.taskDesc; 
        }
    }

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Setup &ast){

    this->inSetup = true; 

    for(auto& stmt : ast.getStatements()){
        stmt->accept(*this); 
    }

    this->inSetup = false; 

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Function::Task &ast){
    auto& ctlNames = this->DivMeta.TaskControllerSplit[ast.getName()]; 
    TaskID ogTaskName = ast.getName(); 
    
  
    for(auto& ctl: ctlNames){
        TaskCopyInfo tci;
        tci.taskPtr = std::make_unique<AstNode::Function::Task>(); 
        // Naming Schema used for now
        TaskID taskName = ogTaskName + "_" + ctl; 
        tci.taskPtr->getName() = taskName; 
        tci.blockStack.push({}); 

        /* 
            Fill out Task header data
        */ 

        auto& derTask = this->DivMeta.ctlMetaData[ctl].taskData[ast.getName()];
        std::unordered_set<DeviceID> derivedDevSet; 
        tci.taskPtr->getParameters() = derTask.parameterList;  
        this->taskCopyMap.emplace(taskName, std::move(tci)); 
    }


    for(auto& stmt : ast.getStatements() ){
        stmt->accept(*this); 
    }

    for(auto& ctl : ctlNames){

        TaskID taskName = ogTaskName + "_" + ctl; 
        auto& taskPtr = this->taskCopyMap[taskName].taskPtr; 
        // Copy the statements for the divided task: 

        auto& statementStack = this->taskCopyMap[taskName].blockStack; 
        if(statementStack.size() == 1){
            taskPtr->getStatements() = std::move(statementStack.top()); 
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
    auto ctlSet = ast.getControllerSplit(); 

    for(auto ctl : ctlSet){
        auto taskName = this->currTask + "_" + ctl; 
        this->taskCopyMap[taskName].blockStack.push({}); 
    }

    for(auto& stmt : ast.getBlock()){
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
        *derivedIf->getCondition() = *ast.getCondition();  
        derivedIf->getBlock() = std::move(newStack); 
    }


   // Do the same for if else and else

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Statement::Declaration &ast){
    auto totalCntSplit = this->DivMeta.TaskControllerSplit[this->currTask]; 
    auto writeCtls = ast.getControllerSplit();
    
    if(this->inSetup){
        // Add the statement to the declaration

        return std::monostate(); 
    }

    for(auto& ctlName : totalCntSplit){
        auto TaskName = this->currTask + "_" + ctlName;
        auto splitDecl = std::make_unique<AstNode::Statement::Declaration>();

        splitDecl->getName() = ast.getName(); 

        auto& typeObj = ast.getType(); 
        splitDecl->getType()->getName() = typeObj->getName(); 

        if(writeCtls.contains(ctlName)){
            if(ast.getValue().has_value()){
                *splitDecl->getValue()->get() = *ast.getValue()->get();
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

    auto ctlList = ast.getControllerSplit(); 
    for(auto& ctlName : ctlList){
        auto TaskName = this->currTask + "_" + ctlName; 
        auto newExpr = std::make_unique<AstNode::Statement::Expression>(); 
        *newExpr->getExpression() = *ast.getExpression(); 

        this->taskCopyMap[TaskName].blockStack.top().push_back(std::move(newExpr)); 
    }
    return std::monostate(); 
}





 


