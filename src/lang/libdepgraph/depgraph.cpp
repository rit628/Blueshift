#include "depgraph.hpp"
#include "ast.hpp"
#include <any>
#include <stdexcept>

using namespace BlsLang;

// Utility Functions: 
void DepGraph::error(std::string msg){
    throw std::invalid_argument("DEPENDENCY GRAPH: " + msg); 
}


SymbolID getPureSymbol(SymbolID &symbol){
    int symPos = symbol.find('%'); 
    if(symPos != std::string::npos){
        return symbol.substr(0, symPos); 
    }

    return symbol; 
}

SymbolID DepGraph::getLastAssignment(SymbolID &symbol){
    SymbolID pureSymbol = getPureSymbol(symbol); 
    int redefCnt = this->oblockCtx.symbolRedefCount[pureSymbol]; 
    if(redefCnt == 1){
        return pureSymbol; 
    }
    return pureSymbol + "%" + std::to_string(redefCnt); 

}

SymbolID DepGraph::getNextAssignment(SymbolID &symbol){
    auto pureSymbol = getPureSymbol(symbol); 
    if(this->oblockCtx.symbolRedefCount.contains(pureSymbol)){
        int redefCnt = this->oblockCtx.symbolRedefCount[pureSymbol]; 
        pureSymbol += "%" + std::to_string(redefCnt + 1); 
    }
    return pureSymbol; 
}

ControllerID DepGraph::extractCtl(std::string deviceConstructor){
    int pos = deviceConstructor.find("::"); 
    if(pos != std::string::npos){
        return deviceConstructor.substr(0, pos); 
    }
    else{
        error("Could not find delimiter in device constructor"); 
    }
}
}

void DepGraph::clearOblockCtx(){
    this->oblockCtx.deviceAliasMap.clear(); 
    this->oblockCtx.symbolicDepGraph.clear(); 
    this->oblockCtx.symbolRedefCount.clear(); 
    this->oblockCtx.symbolicDepGraph.clear(); 
}

// Visitor Functions: 

std::any DepGraph::visit(AstNode::Source &ast){
   this->setupCtx.inSetup = true; 
   ast.getSetup()->accept(*this); 
   this->setupCtx.inSetup = false; 

    // Oblock Code: 
   for(auto& oblock : ast.getOblocks()){
        oblock->accept(*this); 
        clearOblockCtx(); 
   }
}


std::any DepGraph::visit(AstNode::Function::Oblock &oblock){
    auto name = oblock.getName(); 
    auto paramVector = oblock.getParameters(); 
    auto bindedDev = this->globalCtx.oblockDesc[name].bindedDevices; 
    // Establish a mapping between the the paramvectora and bindedDev 
    for(int i = 0 ; i < paramVector.size(); i++){
        this->oblockCtx.deviceAliasMap[paramVector[i]] = bindedDev[i]; 
    }

    // Loop through statements in the oblock
    for(auto& statements : oblock.getStatements()){
        statements->accept(*this); 
    }
}

std::any DepGraph::visit(AstNode::Setup &ast){
    for(auto& statement : ast.getStatements()){
        statement->accept(*this); 
    }
}

std::any DepGraph::visit(AstNode::Statement::Declaration &ast){
    if(this->setupCtx.inSetup){
        DeviceID devName = ast.getName();  
        auto& value = ast.getValue(); 
        if(value.has_value()){
            auto valueNode = value->get()->accept(*this); 
            if(valueNode.type() == typeid(std::string)){
                std::string constructor = std::any_cast<std::string>(valueNode);  
                auto ctlName = extractCtl(constructor); 
                DeviceDependencyNode ddn; 
                ddn.name = devName; 
                ddn.hostController = ctlName; 
                this->globalCtx.deviceDesc[devName] = ddn; 
            }
        }
        else{
            error("Could not resolve the constructor of a device:"); 
        }
    }
    else{
        // Should just be ast.getName() honestly as the redef counter would be 0
        SymbolID var = this->getNextAssignment(ast.getName()); 
        auto& valExpr = ast.getValue(); 
        if(valExpr.has_value()){
            valExpr.value()->accept(*this);
            this->oblockCtx.symbolicDepGraph[var] = this->oblockCtx.processingNode; 
        }
    }
}

std::any DepGraph::visit(AstNode::Expression::Binary &ast){
   if(!this->setupCtx.inSetup){
        if(ast.getOp() == "="){
            // Get Left hand side
            SymbolID varID; 
            this->oblockCtx.asslhs = true; 
            auto var = ast.getLeft()->accept(*this); 
            if(var.type() == typeid(std::string)){
                varID = std::any_cast<std::string>(var); 
            }
            else{
                error("Could not resolve lhs of bin expression as a string!"); 
            }
            this->oblockCtx.asslhs = false; 
            // Processing right Expression
            ast.getRight()->accept(*this);
            this->oblockCtx.symbolRedefCount[varID] +=1 ;
            SymbolID nextAssign = getNextAssignment(varID); 

    
            
        }
        else{
            ast.getLeft(); 
            ast.getRight(); 
        }
   }
}

std::any DepGraph::visit(AstNode::Expression::Function &ast){
    if(this->setupCtx.inSetup){
        OblockDependencyNode odn; 
        odn.name = ast.getName(); 
        for(auto& arg : ast.getArguments()){
            auto obj = arg->accept(*this); 
            if(obj.has_value() && (obj.type() == typeid(std::string))){
                auto devArg = std::any_cast<std::string>(obj);
                odn.bindedDevices.push_back(devArg);  
            }
        }
        this->globalCtx.oblockDesc[odn.name] = odn; 
    }
    else{
        




    }
}

std::any DepGraph::visit(AstNode::Expression::Literal &ast){
    if(this->setupCtx.inSetup){
        std::string devCons;
        if(std::holds_alternative<std::string>(ast.getLiteral())){
            devCons = std::get<std::string>(ast.getLiteral()); 
        }
        else{
            error("Non-string literal found in setup"); 
        }
        return devCons; 
    }
    return true; 
}

std::any DepGraph::visit(AstNode::Expression::Access &ast){
    if(this->setupCtx.inSetup || this->oblockCtx.asslhs){
        return ast.getObject(); 
    }
    else{
        auto symbolName = ast.getObject();
        auto member = ast.getMember(); 
        auto& subscript = ast.getSubscript(); 

        if(member.has_value()){
            auto attr = member.value(); 
            symbolName += "." + attr; 
        } 

        if(subscript.has_value()){
            auto& subExpr = ast.getSubscript().value(); 
            subExpr->accept(*this); 
        }

        // get the last assignment of the symbol in the graph
        SymbolID numName = getLastAssignment(symbolName); 
        this->oblockCtx.processingNode.symDepMap[numName] =
        
    }
}


GlobalCtx& DepGraph::getGlobalCtx(){
    return this->globalCtx; 
}