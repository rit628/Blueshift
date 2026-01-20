#include "symgraph.hpp"
#include "ast.hpp"
#include "error_types.hpp"
#include "include/Common.hpp"
#include "symbolicator.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
using namespace BlsLang; 


/*
*  
*
* Utility Functions for Processing Strings
*
*   
*/ 

/*
    Extract the symbol with the local index: 
*/
std::string extractRawSymbol(const std::string &str){
    int pos = str.find('?'); 
    if(pos != std::string::npos){
        return str.substr(0, pos); 
    }
    return str; 
}

/*
    Extract the complete symbol (removes the local index)
*/
std::string extractFullSymbol(const std::string &str){
    std::string extr1 = extractRawSymbol(str); 
    int pos = extr1.find('%'); 
    if(pos != std::string::npos){
        extr1 = extr1.substr(0, pos); 
    } 

    pos = extr1.find('.'); 
    if(pos != std::string::npos){
        return extr1.substr(0, pos); 
    } 

    return extr1; 
}

std::pair<std::string, std::string> getObjectMember(const std::string &str){
    std::string extr1 = extractRawSymbol(str); 
    int pos = extr1.find('%'); 
    if(pos != std::string::npos){
        extr1 = extr1.substr(0, pos); 
    } 

    pos = extr1.find('.'); 
    if(pos != std::string::npos){
        return std::make_pair(extr1.substr(0, pos) , extr1.substr(pos+1)); 
    } 

    return std::make_pair("", ""); 
}



std::pair<std::unordered_set<SymbolID>, std::unordered_set<DeviceID>> Symgraph::getDeps(AstNode& node){
    std::unordered_set<SymbolID> symbolDeps; 
    std::unordered_set<DeviceID> deviceDeps;    
    
    auto ret = this->symbolicate(node); 
    for(const auto& str : ret){
        // Determine if the raw symbol is a device or a user-defined symbol
        auto rawSymbol = extractFullSymbol(str);  
        if(this->ctx.aliasDevMap.contains(rawSymbol)){
            auto realName = this->ctx.aliasDevMap[rawSymbol]; 
            deviceDeps.insert(realName);  
            //deviceDeps.insert(rawSymbol); 
        }
        else{
            auto currentInitiator = this->ctx.parentData.top()->initiatorPtr->ptr; 
            auto& redefStack = this->ctx.devRedefMap[rawSymbol].deviceRedefStack; 
            for(auto it = redefStack.rbegin(); it != redefStack.rend(); ++it){
                auto candidateInit = (*it).redefStatement->initiatorPtr->ptr; 
          
                auto modString = (*it).symbol; 
                symbolDeps.insert(modString);  
                deviceDeps.insert(this->ctx.symbolMap[modString].symbolDevice.begin(), this->ctx.symbolMap[modString].symbolDevice.end()); 

                if(currentInitiator == candidateInit){
                    break;     
                }                
            }; 
            
        }
    }

    // Add the deps of the initiator statement (if its an IF, For or While)
    auto& parentStatement = this->ctx.parentData.top()->initiatorPtr; 
    if(parentStatement->stype != StatementType::TASK && parentStatement->stype != StatementType::FUNCTION){
        auto& parentDevDeps = this->ctx.parentData.top()->devDeps; 
        for(auto& device : parentDevDeps){
            deviceDeps.insert(device); 
        }
    } 
    

    return std::make_pair(symbolDeps, deviceDeps); 
}

 std::shared_ptr<StatementData> Symgraph::makeStData(AstNode::Statement* ptr, StatementType stype){
    auto stmtPtr = std::make_shared<StatementData>(); 
    stmtPtr->ptr = ptr;
    if(this->ctx.parentData.empty()){
        stmtPtr->initiatorPtr = nullptr; 
    }
    else{
        stmtPtr->initiatorPtr = this->ctx.parentData.top(); 
    }
    stmtPtr->stype = stype; 
    return stmtPtr; 
}

void Symgraph::clearContext(){
    this->ctx.aliasDevMap.clear(); 
    this->ctx.symbolMap.clear(); 
    this->ctx.devRedefMap.clear(); 
    this->ctx.redefCount.clear(); 
    this->ctx.deviceAssignmentSymbols.clear(); 
    // Should be empty anyways but for good measure
    std::stack<std::shared_ptr<StatementData>>().swap(this->ctx.parentData); 
    this->ctx.leftHandAs = false; 
}

/*
*  
*
* Visitor Functions
*
*   
*/ 

BlsObject Symgraph::visit(AstNode::Source& ast){
    auto& taskNodes = ast.getTasks(); 
    for(auto& task : taskNodes){
        auto &src = this->ctx;
        std::string name = task->getName(); 
        this->ctx.currentTask = name; 
        auto& taskCtx = this->taskCtxMap[name]; 
        src.aliasDevMap = taskCtx.devAliasMap; 
        auto taskPtr = makeStData(nullptr, StatementType::TASK);
        this->ctx.parentData.push(taskPtr); 
        task->accept(*this);
        this->ctx.parentData.pop(); 
        this->annotateControllerDivide(); 
        this->clearContext(); 
    }
    return true; 
}

BlsObject Symgraph::visit(AstNode::Function::Task& ast){
    for(auto& stmt : ast.getStatements()){
        auto stmtPtr = makeStData(stmt.get(), StatementType::STATEMENT);
        this->ctx.parentData.push(stmtPtr); 
        stmt->accept(*this); 
        this->ctx.parentData.pop();  
        // Annotate the controller divide for a spe
    }
    return true; 
}

/* 
    IF STATEMENT HANDLER (observe conditional Assignments)
*/
BlsObject Symgraph::visit(AstNode::Statement::If& ast){
    this->ctx.parentData.top()->stype = StatementType::IF; 
    auto& condition = ast.getCondition();
    auto [symbolDeps, devicesDeps] = getDeps(*condition); 
    for(auto& stmt : ast.getBlock()){
        auto stmtPtr = makeStData(stmt.get(), StatementType::STATEMENT);
        stmtPtr->devDeps = devicesDeps;  
        stmtPtr->symbolDeps = symbolDeps; 
        this->ctx.parentData.push(stmtPtr); 
        stmt->accept(*this); 
        this->ctx.parentData.pop(); 
    }

    auto ifCpy = this->ctx.parentData.top(); 
    ifCpy->stype = StatementType::ELSE; 
    this->ctx.parentData.push(ifCpy); 
    for(auto& stmt : ast.getElseBlock()){
        auto stmtPtr = makeStData(stmt.get(), StatementType::STATEMENT);
        this->ctx.parentData.push(stmtPtr); 
        stmt->accept(*this); 
        this->ctx.parentData.pop(); 
    }
    
    this->ctx.parentData.pop(); 

    return std::monostate(); 
}


// Fix the ast to use the ast indexing
BlsObject Symgraph::visit(AstNode::Statement::Declaration &ast){; 
    this->ctx.redefCount.emplace(ast.getName(), 0); 
    std::string symbolName = ast.getName() + "%" + std::to_string(static_cast<int>(ast.getLocalIndex())) + "?0";  

    auto& val = ast.getValue();
    std::unordered_set<SymbolID> symbolDeps; 
    std::unordered_set<DeviceID> deviceDeps; 

    if(val.has_value()){
       auto obj = this->getDeps(*ast.getValue()->get()); 
       symbolDeps = obj.first; 
       deviceDeps = obj.second; 
    }


    DeviceRedef newRedef; 
    newRedef.object = ast.getName(); 
    newRedef.redefStatement = this->ctx.parentData.top();
    newRedef.symbol = symbolName; 

    this->ctx.devRedefMap[ast.getName()].deviceRedefStack.push_back(newRedef);

    this->ctx.symbolMap[symbolName].symbolDeps = symbolDeps; 
    this->ctx.symbolMap[symbolName].symbolDevice = deviceDeps; 
    this->ctx.symbolMap[symbolName].statementInit = this->ctx.parentData.top(); 
    this->ctx.symbolMap[symbolName].symbolName = symbolName; 

    return true; 
}


BlsObject Symgraph::visit(AstNode::Statement::Expression &ast){
    ast.getExpression()->accept(*this); 
    return true; 
}

BlsObject Symgraph::visit(AstNode::Expression::Binary &ast){
    // if equality 
    if(ast.getOp() == "="){
        SymbolID newSymbol; 
        this->ctx.leftHandAs = true; 
        auto ans = resolve(ast.getLeft()->accept(*this)); 
        this->ctx.leftHandAs = false; 

        if(std::holds_alternative<std::string>(ans)){
            newSymbol = std::get<std::string>(ans); 
        }
        else{
            throw std::invalid_argument("Symgraph: Symbol Resolution Failed!"); 
        }

        newSymbol = extractRawSymbol(newSymbol) + "?" + std::to_string(++this->ctx.redefCount[extractRawSymbol(newSymbol)]); 
        auto parentSymbol = extractRawSymbol(newSymbol) + "?0"; 

        // Process right hand side: 
        auto op = this->getDeps(*ast.getRight()); 
        std::unordered_set<SymbolID> symSet = op.first; 
        std::unordered_set<DeviceID> devSet = op.second; 


        // Add the value into the redef queue: 
        auto pureSymbol = extractFullSymbol(newSymbol); 
        DeviceRedef omar; 
        omar.redefStatement = this->ctx.parentData.top(); 
        omar.symbol = newSymbol;
        std::string cuteName = pureSymbol; 

        if(this->ctx.aliasDevMap.contains(pureSymbol)){
            auto [devName, attribute] = getObjectMember(newSymbol); 
            cuteName = devName + "." + attribute; 
            omar.object = devName; 
            omar.attr = attribute; 
            DeviceID devString = this->ctx.aliasDevMap[pureSymbol]; 
            devSet.insert(devString); 
        }
        else{ 
            omar.object = pureSymbol; 
        }

        auto& redefDequeue = this->ctx.devRedefMap[cuteName].deviceRedefStack; 
        auto& currParentStatement = omar.redefStatement->initiatorPtr; 

        if(redefDequeue.empty()){
            redefDequeue.push_back(omar); 
        }
        else{
            auto& lastRedefParent = redefDequeue.back().redefStatement->initiatorPtr;  // Add the statement to the declaration


            // Initiate basic replacement if the sl is 0 
            if(currParentStatement->stype == StatementType::TASK){
                redefDequeue.clear(); 
                redefDequeue.push_back(omar); 
            }
            else{
                // kill all previous redefintions in the same scope level; 
                while(lastRedefParent->ptr == currParentStatement->ptr){
                    auto remRedefObj = redefDequeue.back(); 
                    if(remRedefObj.symbol == parentSymbol){
                        
                    }
                    redefDequeue.pop_back(); 
                    lastRedefParent = redefDequeue.back().redefStatement->initiatorPtr; 
                }
                // If the statement completes an if-else, kill all redefs in previous scope level
                
                if(currParentStatement->stype == StatementType::ELSE){
                    auto grandpa = currParentStatement->initiatorPtr->ptr;
                    // If statement should confirm the existance of a redef in if else.   
                    if(grandpa == lastRedefParent->ptr && lastRedefParent->stype == StatementType::IF){   
                        auto ifRedefCpy = redefDequeue.back(); 
                        while(redefDequeue.back().redefStatement->initiatorPtr->ptr == grandpa){
                            redefDequeue.pop_back(); 
                        }
                        redefDequeue.push_back(ifRedefCpy); 
                    }
                }
                redefDequeue.push_back(omar); 
            }
        }

        auto& symData = this->ctx.symbolMap[newSymbol]; 
        symData.symbolDeps = symSet; 
        symData.symbolDevice = devSet; 
        symData.statementInit = this->ctx.parentData.top(); 


        // If not an assignment on a device then force the declaration as a dependency
        if(!this->ctx.aliasDevMap.contains(pureSymbol)){
            SymbolID declSymbol = extractRawSymbol(newSymbol) + "?0"; 
            if(this->ctx.symbolMap.contains(declSymbol)){
                symSet.insert(declSymbol); 
            }
            else{
                std::cout<<"Something went wrong! undeclared non-device symbol!"<<std::endl; 
            }
        }
        
        this->ctx.symbolMap[newSymbol].symbolDeps = symSet; 
        this->ctx.symbolMap[newSymbol].symbolDevice = devSet; 
        this->ctx.symbolMap[newSymbol].statementInit = this->ctx.parentData.top(); 
        this->ctx.symbolMap[newSymbol].symbolName = newSymbol; 
    }

     return true; 
}

// Used on the lhs side that gets the pure (unordered symbol)
BlsObject Symgraph::visit(AstNode::Expression::Access &ast){
    if(this->ctx.leftHandAs){
        std::string symbol = ast.getObject(); 
        if(ast.getMember().has_value()){
            symbol += "." + ast.getMember().value(); 
        }

        return symbol + "%" + std::to_string(static_cast<int>(ast.getLocalIndex())); 
    }
    return std::monostate(); 
}


/*
*  
*
* DIVISION AND TREE ANNOTATION
*
*   
*/ 

/* 
    Uses the device dependencies to identify how to split an task. An Task can be split into task_[ctl#] for each 
    subset of devices it can be taken from. 
*/ 

struct DevDeps{
    std::unordered_set<DeviceID> deviceDeps; 
    std::unordered_set<ControllerID> ctlDeps; 
}; 

std::vector<std::pair<DeviceID, ControllerID>> Symgraph::findDivisions(){
    
    std::vector<std::pair<DeviceID, ControllerID>> decentralizePair; 
    std::unordered_map<DeviceID, DevDeps> devToDeps; 
    std::unordered_map<DeviceID, DeviceDescriptor> ddesc;  

    // Create the device descriptors map: 
    for(auto& dev: this->taskDescriptorMap[ctx.currentTask].binded_devices){
        ddesc[dev.device_name] = dev; 
    }


    std::unordered_map<DeviceID, SymbolID> invAliasMap; 
    for(auto& pair : this->ctx.aliasDevMap){
        invAliasMap[pair.second] = pair.first; 
    }




    for(auto& pair : this->ctx.devRedefMap ){
        SymbolID vals = pair.first; 
        auto [deviceAlias, attr] = getObjectMember(vals); 
        if(this->ctx.aliasDevMap.contains(deviceAlias)){

            DeviceID devName = this->ctx.aliasDevMap[deviceAlias]; 
            auto& redefStack = pair.second.deviceRedefStack;
            auto& deviceDeps = devToDeps[deviceAlias]; 
            deviceDeps.deviceDeps.insert(devName); 
            std::cout<<"devdep1: "<<ddesc[devName].controller<<std::endl; 
            deviceDeps.ctlDeps.insert(ddesc[devName].controller); 
            

            for(auto it = redefStack.rbegin(); it != redefStack.rend(); ++it){
                auto& deviceDeps = this->ctx.symbolMap[it->symbol].symbolDevice; 
                this->ctx.deviceAssignmentSymbols[devName].insert(it->symbol);
                for(auto& devDep : deviceDeps){
                    auto& devDepCont =  devToDeps[deviceAlias]; 

                    devDepCont.deviceDeps.insert(devDep); 

                    DeviceID realDevName = this->ctx.aliasDevMap[devDep]; 
                    ControllerID ctlName = ddesc[devDep].controller; 
                    std::cout<<"devdep2: "<<ctlName<<std::endl; 
                    devDepCont.ctlDeps.insert(ctlName); 
                }   
            }
        }
    }

    // an task gets split into multiple controllers (but only once for now)
    std::unordered_map<ControllerID, TaskData> derivedInfo;
    std::unordered_set<DeviceID> addedBinds; 

    for(auto& pair : devToDeps){
        ControllerID targController; 
        std::cout<<"Print Controllers: "<<pair.second.ctlDeps.size()<<std::endl; 
        for(auto& i : pair.second.ctlDeps){
            std::cout<<i<<std::endl; 
        }

        if(pair.second.ctlDeps.size() <= 1){
            targController = *pair.second.ctlDeps.begin();  
            decentralizePair.push_back(std::make_pair(pair.first, targController));
        }
        else{
            decentralizePair.push_back(std::make_pair(pair.first, "MASTER")); 
            targController = "MASTER"; 
        }

        // Update the task Descriptors

        this->finalMetadata.TaskControllerSplit[this->ctx.currentTask].insert(targController); 
        
        auto& derTaskDesc = derivedInfo[targController].taskDesc;
        auto& paramList = derivedInfo[targController].parameterList; 

        derTaskDesc.name = this->ctx.currentTask + "_" + targController; 
        auto& devDeps = pair.second.deviceDeps; 
        auto& currDevDesc = ddesc[this->ctx.aliasDevMap[pair.first]]; 

        std::for_each(devDeps.begin(), devDeps.end(), [&](const DeviceID& str){
            auto& readDevDesc = ddesc[str]; 
            std::cout<<"Device desc (SHOULD BE dev): "<<str<<std::endl; 
            
            if(!addedBinds.contains(readDevDesc.device_name)){
                derTaskDesc.inDevices.push_back(readDevDesc);  
                derTaskDesc.binded_devices.push_back(readDevDesc);  
                paramList.push_back(invAliasMap[str]);   
                addedBinds.insert(readDevDesc.device_name); 
            }
        }); 

        if(!addedBinds.contains(currDevDesc.device_name)){
            derTaskDesc.outDevices.push_back(currDevDesc);
            derTaskDesc.binded_devices.push_back(currDevDesc); 
            paramList.push_back(pair.first); 
            addedBinds.insert(currDevDesc.device_name); 
        }
        
        // TODO: Figure out how to handle triggers in decentralization:
    }

    // Transfer the task Data: 
    for(auto& pair : derivedInfo){
        std::cout<<"Omar: "<<pair.first<<std::endl; 
        this->finalMetadata.ctlMetaData[pair.first].ctlName = pair.first; 
        this->finalMetadata.ctlMetaData[pair.first].taskData[this->ctx.currentTask] = pair.second; 
    }

    return decentralizePair; 
}

/*
 First Division
 Second Drill Up Algorithm: Derives subtrees and delegates ownership of statement ptrs using the markations in the StatementData. 
*/ 
 void Symgraph::exhaustiveSymbolFillH(const SymbolID &originSymbol, std::unordered_set<SymbolID>& visitedSymbols, ControllerID &ctl, 
    std::unordered_set<AstNode::Statement*> &visitedStatements){

    auto& symbolData = this->ctx.symbolMap[originSymbol];

    for(auto& item : symbolData.symbolDeps){
        if(!visitedSymbols.contains(item)){
            exhaustiveSymbolFillH(item, visitedSymbols, ctl, visitedStatements); 
        }
    }

    exhaustiveStatementFillH(symbolData.statementInit->initiatorPtr, visitedSymbols, ctl, visitedStatements); 

    visitedSymbols.insert(originSymbol); 
    symbolData.statementInit->ptr->getControllerSplit().insert(ctl); 

 }


// Hash already visited statements to make quicker? 
 void Symgraph::exhaustiveStatementFillH(std::shared_ptr<StatementData> st,  std::unordered_set<SymbolID>& visitedSymbols, ControllerID &ctl,
    std::unordered_set<AstNode::Statement*> &visitedStatements){

    if(st->stype == StatementType::TASK){
        return; 
    }

    if(!visitedStatements.contains(st->initiatorPtr->ptr)){
        exhaustiveStatementFillH(st->initiatorPtr,  visitedSymbols, ctl, visitedStatements); 
        visitedStatements.insert(st->initiatorPtr->ptr); 
    }
   

    for(auto& symbolPtr : st->symbolDeps){
        if(!visitedSymbols.contains(symbolPtr)){
            exhaustiveSymbolFillH(symbolPtr, visitedSymbols, ctl, visitedStatements); 
        }
    }
 }


// Fill in the annotations with the controllers to divide each statement into
 void Symgraph::annotateControllerDivide(){
    // Device Pairings: 
    auto TaskDivisions = this->findDivisions(); 

    // Loop through device list and add into task divisions
    for(auto& [device,controller] : TaskDivisions){
        for(SymbolID sym : this->ctx.deviceAssignmentSymbols[device]){
            std::unordered_set<SymbolID> visitedSymbols; 
            std::unordered_set<AstNode::Statement*> visitedStatements; 
            exhaustiveSymbolFillH(sym, visitedSymbols, controller, visitedStatements); 
        }
    }
 }

  DividerMetadata Symgraph::getDivisionData(){
    return this->finalMetadata; 
 }

