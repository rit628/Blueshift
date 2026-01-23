#include "depgraph.hpp"

#include <stdexcept>
#include <string>
#include <variant>

using namespace BlsLang; 

/* 
UTILITY FUNCTION
*/

namespace  {
    void error(std::string message){
        throw std::invalid_argument("DEPENDENCY GRAPH: " + message); 
    }
    
    
    ControllerID extractCtls(const std::string& deviceCons){
        int pos = deviceCons.find("::"); 
        if(pos != std::string::npos){
            return deviceCons.substr(0, pos); 
        }
        // error("Device constructor contains no device name"); 
        return ""; 
    }
}

GlobalContext& DepGraph::getGlobalContext(){
    return this->globalCtx; 
}

void DepGraph::printGlobalContext(){
    auto& taskMap =  this->globalCtx.taskConnections; 

    for(auto& pair : taskMap){
        std::cout<<"Task: "<<pair.first<<std::endl; 
        std::cout<<"In Devices: "<<std::endl; 
        for(auto& dev : pair.second.inDeviceList){
            std::cout<<"*"<<dev<<std::endl; 
        }
        std::cout<<"Out Devices: "<<std::endl; 
        for(auto& dev : pair.second.outDeviceList){
            std::cout<<"*"<<dev<<std::endl; 
        }
        
        std::cout<<"-------------------------"<<std::endl; 
    }
}

void DepGraph::clearTaskCtx(){
    this->taskCtx.operatingTask = ""; 
    this->taskCtx.devAliasMap.clear();
    this->taskCtx.tempDevices.clear();  
}

bool DepGraph::isDevice(const SymbolID& isDevice){
    return this->taskCtx.devAliasMap.contains(isDevice); 
}


/*
VISITOR FUNCTIONS
*/

BlsObject DepGraph::visit(AstNode::Source& ast) {
    this->setupCtx.inSetup = true; 
    ast.getSetup()->accept(*this); 
    this->setupCtx.inSetup = false; 

    auto& taskList = ast.getTasks(); 
    for(auto& task : taskList){
        task->accept(*this); 
        this->taskCtxList.push_back(this->taskCtx); 
        clearTaskCtx(); 
    }

    return true; 
}

BlsObject DepGraph::visit(AstNode::Setup& ast) {
   for(auto& statements : ast.getStatements()){
        statements->accept(*this); 
   }

   return true; 
}

BlsObject DepGraph::visit(AstNode::Function::Task& ast){
    auto& name = ast.getName(); 
    this->taskCtx.operatingTask = name; 
    auto& params = ast.getParameters(); 

    if(this->globalCtx.taskConnections.contains(name)){

        auto realDev = this->globalCtx.taskConnections[name].bindedDevices;
        
        for(int i = 0; i < params.size(); i++){
            this->taskCtx.devAliasMap[params[i]] = realDev[i]; 
        }

        for(auto& statement : ast.getStatements()){ 
            statement->accept(*this); 
        }
    }
   
    return true; 
}

BlsObject DepGraph::visit(AstNode::Statement::Declaration& ast) {
    if(this->setupCtx.inSetup){ 
        //  We assume declarartions in a setup context are device inits
        AstDeviceDesc devDesc; 
        DeviceID dev = ast.getName(); 
        devDesc.name = dev; 
        auto& valStatement = ast.getValue(); 
        if(valStatement.has_value()){
            auto container = resolve(valStatement.value()->accept(*this)); 
            if(std::holds_alternative<std::string>(container)){
                std::string constructor = std::get<std::string>(container); 
                std::string ctl = extractCtls(constructor); 
                devDesc.ctl_name = ctl; 
                this->globalCtx.deviceConnections[dev] = devDesc; 
            }
        }
    }
    else{
        // Dependency graph production outside setup 
        SymbolID name = ast.getName(); 
        if(ast.getValue().has_value()){
            ast.getValue().value()->accept(*this);  
        }
    }

    return true; 
}


BlsObject DepGraph::visit(AstNode::Expression::Binary &ast){
    if(ast.getOp() == "="){
       this->taskCtx.isReading = false;  
       ast.getLeft()->accept(*this); 
       this->taskCtx.isReading = true; 

       ast.getRight()->accept(*this); 
    }
    else if(ast.getOp() == "+="){
        this->taskCtx.isRW = true; 
        ast.getLeft()->accept(*this); 
        this->taskCtx.isRW = false; 
        ast.getRight()->accept(*this); 
    }
    else{
        ast.getLeft()->accept(*this); 
        ast.getRight()->accept(*this); 
    }
    return true; 
}


BlsObject DepGraph::visit(AstNode::Statement::Expression &ast){
    ast.getExpression()->accept(*this); 
    return true; 
}

BlsObject DepGraph::visit(AstNode::Expression::Function& ast) {
    if(this->setupCtx.inSetup){
        AstTaskDesc taskDesc; 
        TaskID task = ast.getName(); 
        taskDesc.name = task; 
        auto& argList = ast.getArguments(); 
        for(auto& dev : argList){
            auto devArg = resolve(dev->accept(*this)); 
            if(std::holds_alternative<std::string>(devArg)){
                auto bindDev = std::get<std::string>(devArg); 
                taskDesc.bindedDevices.push_back(bindDev); 
            }
            else{
                error("Could not derive task argument as string!"); 
            }
        }
        this->globalCtx.taskConnections[task] = taskDesc; 
    }
    else{
        auto& argList = ast.getArguments(); 
        for(auto& statement : argList){
            statement->accept(*this); 
        }
    }

    return true; 
}


BlsObject DepGraph::visit(AstNode::Expression::Access& ast) {
    if(this->setupCtx.inSetup){
        return ast.getObject(); 
    }
    else{
        auto obj = ast.getObject(); 
        if(isDevice(obj)){
            auto& realDev = this->taskCtx.devAliasMap[obj]; 
            this->taskCtx.tempDevices.insert(realDev); 
        }

        if(ast.getSubscript().has_value()){
            ast.getSubscript().value()->accept(*this); 
        }

        
        if(this->taskCtx.isRW){
            auto &targ = this->taskCtx.operatingTask; 
            for(const DeviceID& dev : this->taskCtx.tempDevices){
                this->globalCtx.taskConnections[targ].inDeviceList.insert(dev); 
                this->globalCtx.taskConnections[targ].outDeviceList.insert(dev); 
            }
        }
        else if(this->taskCtx.isReading){
            auto &targ = this->taskCtx.operatingTask; 
            for(const DeviceID& dev : this->taskCtx.tempDevices){
                this->globalCtx.taskConnections[targ].inDeviceList.insert(dev); 
            }
        }
        else{
            auto &targ = this->taskCtx.operatingTask; 
            for(const DeviceID& dev : this->taskCtx.tempDevices){
                this->globalCtx.taskConnections[targ].outDeviceList.insert(dev); 
            }
        }

        this->taskCtx.tempDevices.clear(); 
    }
    return true; 
}


BlsObject DepGraph::visit(AstNode::Expression::Literal& ast) {
    if(this->setupCtx.inSetup){
        if (std::holds_alternative<std::string>(ast.getLiteral())) {
            return std::get<std::string>(ast.getLiteral());
        }
    }
   
    return true; 
}

BlsObject DepGraph::visit(AstNode::Statement::If& ast) {
    ast.getCondition()->accept(*this); 

    for(auto& ifStatement : ast.getBlock()){
        ifStatement->accept(*this); 
    }

    for(auto& ifElseStatement : ast.getElseIfStatements()){
        ifElseStatement->accept(*this); 
    }

    for(auto& elseStatement :  ast.getElseBlock()){
        elseStatement->accept(*this); 
    }
    return true; 
    
}

BlsObject DepGraph::visit(AstNode::Statement::For& ast) {
    if(ast.getCondition().has_value()){
        ast.getCondition()->get()->accept(*this); 
    }

    if(ast.getInitStatement().has_value()){
        ast.getInitStatement().value()->accept(*this); 
    }

    if(ast.getIncrementExpression().has_value()){
        ast.getIncrementExpression().value()->accept(*this); 
    }

    for(auto& sm : ast.getBlock()){
        sm->accept(*this); 
    }
    return true; 
    
}

BlsObject DepGraph::visit(AstNode::Statement::While& ast) {
    ast.getCondition()->accept(*this); 
    for(auto &sm : ast.getBlock()){
        sm->accept(*this); 
    }
    return true; 
}

BlsObject DepGraph::visit(AstNode::Expression::Group& ast) {
    ast.getExpression()->accept(*this); 
    return true; 
}


BlsObject DepGraph::visit(AstNode::Expression::Unary& ast){
    if(ast.getOp() == "++" || ast.getOp() == "--"){
        this->taskCtx.isRW = true; 
        ast.getExpression()->accept(*this); 
        this->taskCtx.isRW = false; 
    }
    else{
        ast.getExpression()->accept(*this); 
    }
    return true; 
}

/*

BlsObject DepGraph::visit(AstNode::Function::Procedure& ast) {
    
}

BlsObject DepGraph::visit(AstNode::Function::Task& ast) {
    
}

BlsObject DepGraph::visit(AstNode::Statement::Return& ast) {
    
}

BlsObject DepGraph::visit(AstNode::Statement::Continue& ast) {

}

BlsObject DepGraph::visit(AstNode::Statement::Break& ast) {

}



BlsObject DepGraph::visit(AstNode::Statement::Expression& ast) {

}

BlsObject DepGraph::visit(AstNode::Expression::Binary& ast) {

}

BlsObject DepGraph::visit(AstNode::Expression::Unary& ast) {

}

BlsObject DepGraph::visit(AstNode::Expression::Group& ast) {

}

BlsObject DepGraph::visit(AstNode::Expression::Method& ast) {

}


BlsObject DepGraph::visit(AstNode::Expression::List& ast) {

}

BlsObject DepGraph::visit(AstNode::Expression::Set& ast) {
    
}

BlsObject DepGraph::visit(AstNode::Expression::Map& ast) {

}

BlsObject DepGraph::visit(AstNode::Specifier::Type& ast) {

    
}
*/ 


