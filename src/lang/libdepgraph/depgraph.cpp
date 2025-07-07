#include "depgraph.hpp"

#include <stdexcept>

using namespace BlsLang; 

/* 
UTILITY FUNCTION
*/

void error(std::string message){
    throw std::invalid_argument("DEPENDENCY GRAPH: " + message); 
}


ControllerID extractCtls(const std::string& deviceCons){
    int pos = deviceCons.find("::"); 
    if(pos != std::string::npos){
        return deviceCons.substr(0, pos); 
    }
    error("Device constructor contains no device name"); 
    return ""; 
}

GlobalContext& DepGraph::getGlobalContext(){
    return this->globalCtx; 
}

void DepGraph::printGlobalContext(){
    auto& oblockMap =  this->globalCtx.oblockConnections; 

    for(auto& pair : oblockMap){
        std::cout<<"Oblock: "<<pair.first<<std::endl; 
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

void DepGraph::clearOblockCtx(){
    this->oblockCtx.operatingOblock = ""; 
    this->oblockCtx.devAliasMap.clear();
    this->oblockCtx.tempDevices.clear();  
}

bool DepGraph::isDevice(const SymbolID& isDevice){
    return this->oblockCtx.devAliasMap.contains(isDevice); 
}


/*
VISITOR FUNCTIONS
*/

BlsObject DepGraph::visit(AstNode::Source& ast) {
    this->setupCtx.inSetup = true; 
    ast.getSetup()->accept(*this); 
    this->setupCtx.inSetup = false; 

    auto& oblockList = ast.getOblocks(); 
    for(auto& oblock : oblockList){
        oblock->accept(*this); 
        this->oblockCtxList.push_back(this->oblockCtx); 
        clearOblockCtx(); 
    }

    return true; 
}

BlsObject DepGraph::visit(AstNode::Setup& ast) {
   for(auto& statements : ast.getStatements()){
        statements->accept(*this); 
   }

   return true; 
}

BlsObject DepGraph::visit(AstNode::Function::Oblock& ast){
    auto& name = ast.getName(); 
    this->oblockCtx.operatingOblock = name; 
    auto& params = ast.getParameters(); 

    if(this->globalCtx.oblockConnections.contains(name)){

        auto realDev = this->globalCtx.oblockConnections[name].bindedDevices;
        
        for(int i = 0; i < params.size(); i++){
            this->oblockCtx.devAliasMap[params[i]] = realDev[i]; 
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
            else{
                error("Device declaration has no constructor!");
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
       this->oblockCtx.isReading = false;  
       ast.getLeft()->accept(*this); 
       this->oblockCtx.isReading = true; 

       ast.getRight()->accept(*this); 
    }
    else if(ast.getOp() == "+="){
        this->oblockCtx.isRW = true; 
        ast.getLeft()->accept(*this); 
        this->oblockCtx.isRW = false; 
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
        AstOblockDesc oblockDesc; 
        OblockID oblock = ast.getName(); 
        oblockDesc.name = oblock; 
        auto& argList = ast.getArguments(); 
        for(auto& dev : argList){
            auto devArg = resolve(dev->accept(*this)); 
            if(std::holds_alternative<std::string>(devArg)){
                auto bindDev = std::get<std::string>(devArg); 
                oblockDesc.bindedDevices.push_back(bindDev); 
            }
            else{
                error("Could not derive oblock argument as string!"); 
            }
        }
        this->globalCtx.oblockConnections[oblock] = oblockDesc; 
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
            auto& realDev = this->oblockCtx.devAliasMap[obj]; 
            this->oblockCtx.tempDevices.insert(realDev); 
        }

        if(ast.getSubscript().has_value()){
            ast.getSubscript().value()->accept(*this); 
        }

        
        if(this->oblockCtx.isRW){
            auto &targ = this->oblockCtx.operatingOblock; 
            for(const DeviceID& dev : this->oblockCtx.tempDevices){
                this->globalCtx.oblockConnections[targ].inDeviceList.insert(dev); 
                this->globalCtx.oblockConnections[targ].outDeviceList.insert(dev); 
            }
        }
        else if(this->oblockCtx.isReading){
            auto &targ = this->oblockCtx.operatingOblock; 
            for(const DeviceID& dev : this->oblockCtx.tempDevices){
                this->globalCtx.oblockConnections[targ].inDeviceList.insert(dev); 
            }
        }
        else{
            auto &targ = this->oblockCtx.operatingOblock; 
            for(const DeviceID& dev : this->oblockCtx.tempDevices){
                this->globalCtx.oblockConnections[targ].outDeviceList.insert(dev); 
            }
        }

        this->oblockCtx.tempDevices.clear(); 
    }
    return true; 
}


BlsObject DepGraph::visit(AstNode::Expression::Literal& ast) {
    if(this->setupCtx.inSetup){
        return std::get<std::string>(ast.getLiteral()); 
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
        this->oblockCtx.isRW = true; 
        ast.getExpression()->accept(*this); 
        this->oblockCtx.isRW = false; 
    }
    else{
        ast.getExpression()->accept(*this); 
    }
    return true; 
}

/*

BlsObject DepGraph::visit(AstNode::Function::Procedure& ast) {
    
}

BlsObject DepGraph::visit(AstNode::Function::Oblock& ast) {
    
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


