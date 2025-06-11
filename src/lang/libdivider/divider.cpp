#include "divider.hpp"
#include "ast.hpp"
#include "symbolicator.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
using namespace BlsLang; 

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



std::pair<std::unordered_set<SymbolID>, std::unordered_set<DeviceID>> Divider::getDeps(AstNode& node){
    std::unordered_set<SymbolID> symbolDeps; 
    std::unordered_set<DeviceID> deviceDeps;    
    
    auto ret = this->symbolicate(node); 
    for(const auto& str : ret){
        // Determine if the raw symbol is a device or a user-defined symbol
        auto rawSymbol = extractFullSymbol(str); 
        if(this->ctx.aliasDevMap.contains(rawSymbol)){
            auto realName = this->ctx.aliasDevMap[rawSymbol]; 
            deviceDeps.insert(realName); 
        }
        else{
            std::string modString = str + "?" + std::to_string(this->ctx.redefCount[str]); 
            symbolDeps.insert(modString); 
            deviceDeps.insert(this->ctx.symbolMap[modString].symbolDevice.begin(), this->ctx.symbolMap[modString].symbolDevice.end()); 
        }
    }

    return std::make_pair(symbolDeps, deviceDeps); 
}

 std::shared_ptr<StatementData> Divider::makeStData(AstNode* ptr, StatementType stype){
    auto stmtPtr = std::make_shared<StatementData>(); 
    stmtPtr->ptr = reinterpret_cast<uintptr_t>(ptr);
    if(this->ctx.parentData.empty()){
        stmtPtr->initiatorPtr = nullptr; 
    }
    else{
        stmtPtr->initiatorPtr = this->ctx.parentData.top(); 
    }
    stmtPtr->stype = stype; 
    return stmtPtr; 
}

BlsObject Divider::visit(AstNode::Source& ast){
    auto& oblockNodes = ast.getOblocks(); 
    for(auto& oblock : oblockNodes){
        auto &src = this->ctx;
        std::string name = oblock->getName(); 
        auto& oblockCtx = this->oblockCtxMap[name]; 
        src.aliasDevMap = oblockCtx.devAliasMap; 
        auto oblockPtr = makeStData(oblock.get(), StatementType::OBLOCK);
        std::cout<<"Made it"<<std::endl; 
        this->ctx.parentData.push(oblockPtr); 
        oblock->accept(*this);
        this->divContextMap[name] = this->ctx; 
        this->ctx.parentData.pop(); 
    }
    return true; 
}

BlsObject Divider::visit(AstNode::Function::Oblock& ast){
    for(auto& stmt : ast.getStatements()){
        auto stmtPtr = makeStData(stmt.get(), StatementType::STATEMENT);
        this->ctx.parentData.push(stmtPtr); 
        stmt->accept(*this); 
        this->ctx.parentData.pop(); 
        this->ctx.currScopeLevel = 0; 
    }
    return true; 
}

/* 
    IF STATEMENT HANDLER (observe conditional Assignments)
*/
BlsObject Divider::visit(AstNode::Statement::If& ast){
    this->ctx.parentData.top()->stype = StatementType::IF; 
    auto& condition = ast.getCondition();
    auto [symbolData, deviceID] = getDeps(*condition); 
    for(auto& stmt : ast.getBlock()){
        auto stmtPtr = makeStData(stmt.get(), StatementType::STATEMENT); 
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
BlsObject Divider::visit(AstNode::Statement::Declaration &ast){
    this->ctx.parentData.pop(); 
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

    this->ctx.symbolMap[symbolName].symbolDeps = symbolDeps; 
    this->ctx.symbolMap[symbolName].symbolDevice = deviceDeps; 
    this->ctx.symbolMap[symbolName].statementInit = this->ctx.parentData.top(); 

    return true; 
}


BlsObject Divider::visit(AstNode::Statement::Expression &ast){
    ast.getExpression()->accept(*this); 
    return true; 
}

BlsObject Divider::visit(AstNode::Expression::Binary &ast){
    // if equality 
this->ctx.parentData.pop(); 
    if(ast.getOp() == "="){
        std::cout<<"Made it to assignment"<<std::endl; 
        SymbolID newSymbol; 
        this->ctx.leftHandAs = true; 
        auto ans = resolve(ast.getLeft()->accept(*this)); 
        this->ctx.leftHandAs = false; 

        if(std::holds_alternative<std::string>(ans)){
            newSymbol = std::get<std::string>(ans); 
        }
        else{
            throw std::invalid_argument("Divider: Symbol Resolution Failed!"); 
        }

        newSymbol = extractRawSymbol(newSymbol) + "?" + std::to_string(++this->ctx.redefCount[extractRawSymbol(newSymbol)]); 

        // Process right hand side: 
        auto op = this->getDeps(*ast.getRight()); 
        std::unordered_set<SymbolID> symSet = op.first; 
        std::unordered_set<DeviceID> devSet = op.second; 

        // Check if the device is a devtype
        auto pureSymbol = extractFullSymbol(newSymbol); 
        if(this->ctx.aliasDevMap.contains(pureSymbol)){
            devSet.insert(this->ctx.aliasDevMap[pureSymbol]); 

            auto [devName, attribute] = getObjectMember(newSymbol); 
            auto cuteName = devName + "." + attribute; 
            DeviceRedef omar; 
            omar.device = devName; 
            omar.attr = attribute; 
            omar.redefStatement = this->ctx.parentData.top(); 
            omar.symbol = newSymbol;

            auto& redefDequeue = this->ctx.devRedefMap[cuteName].deviceRedefStack; 
            auto& currParentStatement = omar.redefStatement->initiatorPtr; 
            auto& lastRedefParent = redefDequeue.back().redefStatement->initiatorPtr; 

            // Initiate basic replacement if the sl is 0 
            if(currParentStatement->stype == StatementType::OBLOCK){
                redefDequeue.clear(); 
                redefDequeue.push_back(omar); 
            }
            else{
                // kill all previous redefintions in the same scope level; 
                while(lastRedefParent->ptr == currParentStatement->ptr){
                    redefDequeue.pop_back(); 
                }
                // If the statement completes an if-else, kill all redefs in previous scope level
                auto grandpa = currParentStatement->initiatorPtr->ptr; 
                if(currParentStatement->stype == StatementType::ELSE){
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
        
        this->ctx.symbolMap[newSymbol].symbolDeps = symSet; 
        this->ctx.symbolMap[newSymbol].symbolDevice = devSet; 
        this->ctx.symbolMap[newSymbol].statementInit = this->ctx.parentData.top(); 
     
    }

     return true; 
}

BlsObject Divider::visit(AstNode::Expression::Access &ast){
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
DRILL UP FUNCTION: (I KNOW THIS IS SLOW I WILL MAKE IT O(N) 
INSTEAD OF OF O(MN))

*/











