#pragma once

#include "../libdepgraph/depgraph.hpp"
#include "ast.hpp"
#include "symbolicator.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <deque>
#include <stack> 

// Contains a mapping from each statement to the device is can be split to. 
using StatementPtr = uint32_t; 

namespace BlsLang{
    enum class StatementType{
        EXPR, 
        OBLOCK,
        IF, 
        ELSE,
        FOR, 
        WHILE, 
        FUNCTION, 
        STATEMENT, 
    }; 

    struct StatementData{
        uintptr_t ptr; 
        std::shared_ptr<StatementData> initiatorPtr = nullptr; 
        StatementType stype; 
        std::unordered_set<DeviceID> devDeps; 
    }; 

    struct SymbolData{
        std::unordered_set<SymbolID> symbolDeps; 
        std::unordered_set<DeviceID> symbolDevice; 
        std::shared_ptr<StatementData> statementInit; 
        int parentLevel = 0; 
    }; 

    struct DeviceRedef{
        SymbolID symbol; 
        DeviceID device; 
        std::string attr;
        std::shared_ptr<StatementData> redefStatement; 
    }; 

    struct DeviceRedefsCtx{
        std::deque<DeviceRedef> deviceRedefStack; 
    }; 

    // Current Context
    struct divContext{
        // Contains data about the oblock context
        std::unordered_map<SymbolID, DeviceID> aliasDevMap; 
        std::stack<std::shared_ptr<StatementData>> parentData; 
        
        std::unordered_map<SymbolID, SymbolData> symbolMap; 

        std::unordered_map<SymbolID, int> redefCount; 
        // Assignment Statement Specifics: 
        SymbolID processingSymbol; 
        // Determines if the current node is a left hand assignment (special at access)
        bool leftHandAs = false; 
        // Vector of redefs per scope
        int currScopeLevel = 0; 
    
        // Device RedefContext
        std::unordered_map<DeviceID, DeviceRedefsCtx> devRedefMap; 

        // Processing Symbol: 
        std::shared_ptr<StatementData> currentSymbol; 
    }; 


    class Divider : public Printer{
        private: 
            std::unordered_map<OblockID, OblockContext> oblockCtxMap; 
            divContext ctx; 
            std::unordered_map<OblockID, divContext> divContextMap; 
            std::unordered_map<DeviceID, AstDeviceDesc> devContMap; 

            std::shared_ptr<StatementData> makeStData(AstNode* ptr, StatementType stype);
            std::pair<std::unordered_set<SymbolID>, std::unordered_set<DeviceID>> getDeps(AstNode& ast);
        
            static std::unordered_set<SymbolID> symbolicate(AstNode& ast) {
                Symbolicator s;
                ast.accept(s);
                return s.getSymbols();
            }


        
        public: 
            Divider() : 
            Printer(std::cout){}

           ~Divider() = default; 
            
            BlsObject visit(AstNode::Source& ast) override; 
            BlsObject visit(AstNode::Function::Oblock& ast) override; 
            BlsObject visit(AstNode::Statement::Declaration& ast) override;
            BlsObject visit(AstNode::Statement::Expression& ast) override; 
            BlsObject visit(AstNode::Statement::If &ast) override; 
            
            BlsObject visit(AstNode::Expression::Access& ast) override; 
            BlsObject visit(AstNode::Expression::Binary &ast) override; 
       
            /*
                Debug and Drill Function Prototypes
            */
            void setOblocks(std::unordered_map<OblockID, OblockContext> oblockCtxMap, std::unordered_map<DeviceID, AstDeviceDesc>& deviceDescriptors){
                this->oblockCtxMap = oblockCtxMap; 
            }

            std::unordered_map<OblockID, divContext>& getContextsDebug(){
                return this->divContextMap; 
            }

            void divideDevice(); 

            
            

         
    }; 
} 