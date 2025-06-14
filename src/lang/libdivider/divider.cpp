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
        auto& ctlData = pair.second; 
        auto srcCtl = std::make_unique<AstNode::Source>(); 
        ControllerSource ControllerCarrier; 
        ControllerCarrier.ctlSource = std::move(srcCtl); 
        this->ctlSourceMap[pair.first] = std::move(ControllerCarrier); 
    }
 
    ast.getSetup()->accept(*this); 

  
    for(auto& OblockPtr : ast.getOblocks()){
        this->inOblock = true; 
        OblockPtr->accept(*this); 
        this->inOblock = false; 
    }
  

    // Populate the source of each object
    for(auto& pair : this->DivMeta.ctlMetaData){
        auto& ctlData = pair.second; 
        auto& srcObj = this->ctlSourceMap[ctlData.ctlName];
        auto& srcPtr = srcObj.ctlSource; 
        srcPtr->getOblocks() = std::move(srcObj.oblock_list); 
        
        auto srcCtl = std::make_unique<AstNode::Source>(); 
        srcCtl->getSetup()->getStatements() = std::move(srcObj.setup_statements); 
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

BlsObject Divider::visit(AstNode::Function::Oblock &ast){
    auto& ctlNames = this->DivMeta.OblockControllerSplit[ast.getName()]; 
    OblockID ogOblockName = ast.getName(); 
    
  
    for(auto& ctl: ctlNames){
        OblockCopyInfo oci;
        oci.oblockPtr = std::make_unique<AstNode::Function::Oblock>(); 
        // Naming Schema used for now
        OblockID oblockName = ogOblockName + "_" + ctl; 
        oci.oblockPtr->getName() = oblockName; 
        oci.blockStack.push({}); 

        /* 
            Fill out Oblock header data
        */ 

        DerivedOblock derOblock = this->DivMeta.ctlMetaData[ctl].oblockData[ast.getName()];
        std::vector<std::string> newParameters; 
        std::vector<std::unique_ptr<AstNode::Specifier::Type>> newParameterTypes; 

        for(auto& dev : ast.getParameters()){
            if(derOblock.devices.contains(dev)){    
                newParameters.push_back(dev); 
            } 
        }
        oci.oblockPtr->getParameters() = newParameters; 
        this->oblockCopyMap.emplace(oblockName, std::move(oci)); 
    }


    for(auto& stmt : ast.getStatements() ){
        stmt->accept(*this); 
    }

    for(auto& ctl : ctlNames){

        OblockID oblockName = ogOblockName + "_" + ctl; 
        auto& oPtr = this->oblockCopyMap[oblockName].oblockPtr; 
        // Copy the statements for the divided oblock: 

        auto& statementStack = this->oblockCopyMap[oblockName].blockStack; 
        if(statementStack.size() == 1){
            oPtr->getStatements() = std::move(statementStack.top()); 
            statementStack.pop(); 
        }
        else{
            std::cout<<"Divider Error: Statement stack at size: "<<statementStack.size()<<" when copied into Oblock!"<<std::endl; 
        }
        
        this->ctlSourceMap.at(ctl).oblock_list.push_back(std::move(oPtr));
        
    }


    return std::monostate(); 
}





BlsObject Divider::visit(AstNode::Statement::If &ast){
    auto ctlSet = ast.getControllerSplit(); 

    for(auto ctl : ctlSet){
        auto oblockName = this->currOblock + "_" + ctl; 
        this->oblockCopyMap[oblockName].blockStack.push({}); 
    }

    for(auto& stmt : ast.getBlock()){
        stmt->accept(*this); 
    }

    for(auto& ctl : ctlSet){
        // Pop and add the present controller split statement
        auto oblockName = this->currOblock + "_" + ctl; 
        auto& derOblockMap = this->oblockCopyMap[oblockName].blockStack; 
        auto newStack = std::move(derOblockMap.top()); 
        derOblockMap.pop(); 
        
        // make a new statement object
        auto derivedIf = std::make_unique<AstNode::Statement::If>(); 
        *derivedIf->getCondition() = *ast.getCondition();  
        derivedIf->getBlock() = std::move(newStack); 
    }


   // Do the same for if else and else

    return std::monostate(); 
}

BlsObject Divider::visit(AstNode::Statement::Declaration &ast){
    auto ctls = ast.getControllerSplit();
    
    if(this->inSetup){
        // Add the statement to the declaration

        return std::monostate(); 
    }

    for(auto& ctlName : ctls){
        auto OblockName = this->currOblock + "_" + ctlName;
        auto splitDecl = std::make_unique<AstNode::Statement::Declaration>();

        //*splitDecl = *ast; 
        splitDecl->getName() = ast.getName(); 

        auto& typeObj = ast.getType(); 
        splitDecl->getType()->getName() = typeObj->getName(); 

        
        if(ast.getValue().has_value()){
            *splitDecl->getValue()->get() = *ast.getValue()->get();
        }
        this->oblockCopyMap[OblockName].blockStack.top().push_back(std::move(splitDecl));
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
        auto OblockName = this->currOblock + "_" + ctlName; 
        auto newExpr = std::make_unique<AstNode::Statement::Expression>(); 
        *newExpr->getExpression() = *ast.getExpression(); 

        this->oblockCopyMap[OblockName].blockStack.top().push_back(std::move(newExpr)); 
    }
    return std::monostate(); 
}





 


