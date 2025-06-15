#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "../libsymgraph/symgraph.hpp"
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>


namespace BlsLang{

    // Derived Oblock Metadata for performing the split
    struct DerivedOblock{
        std::unordered_set<DeviceID> devices; 
        OblockID oblockName; 
        std::unordered_set<ControllerID> dividedCtls; 
    }; 

    struct ControllerMetadata{
        // Oblock List
        ControllerID ctlName; 
        std::unordered_map<OblockID, DerivedOblock> oblockData; 
        std::vector<DeviceID> devices;  
    }; 

    // Data from dep graph needed to function (in addition to the annotation)
    struct DividerMetadata{
        std::unordered_map<ControllerID, ControllerMetadata> ctlMetaData; 
        std::unordered_map<OblockID ,std::unordered_set<ControllerID>> OblockControllerSplit; 
    }; 


    // Controller Source 
    struct ControllerSource{
        std::unique_ptr<AstNode::Source> ctlSource; 
        std::vector<std::unique_ptr<AstNode::Statement>> setup_statements; 
        std::vector<std::unique_ptr<AstNode::Function>> oblock_list;  
    }; 


    struct OblockCopyInfo{
        std::stack<std::vector<std::unique_ptr<AstNode::Statement>>> blockStack; 
        std::unordered_map<SymbolID,  bool> symbolDeclMap; 
        std::unique_ptr<AstNode::Function::Oblock> oblockPtr;

        OblockCopyInfo() = default; 

        OblockCopyInfo(const OblockCopyInfo&) = delete;
        OblockCopyInfo& operator=(const OblockCopyInfo&) = delete;

        OblockCopyInfo(OblockCopyInfo&&) = default;
        OblockCopyInfo& operator=(OblockCopyInfo&&) = default;

    }; 


    class Divider : public Printer{

    private: 
        // What is to be returned by the divider
        std::unordered_map<ControllerID, ControllerSource> ctlSourceMap;  

        /*
        * Divider Metadata (Produced by Symbolic Dependency Graph)
        */

        DividerMetadata DivMeta; 

        /*
            Divider Context
        */
        std::unordered_map<OblockID, OblockCopyInfo> oblockCopyMap; 
        bool inOblock = true; 
        OblockID currOblock; 
        bool inSetup = false; 
        // Used when visiting an expression node (with no access to splits)
        OblockID subOblockName; 
        
        
    public: 
        Divider():Printer(std::cout) {

        }

        BlsObject visit(AstNode::Source& ast) override; 
        BlsObject visit(AstNode::Setup &ast) override; 
        BlsObject visit(AstNode::Function::Oblock& ast) override; 
        BlsObject visit(AstNode::Statement::Declaration& ast) override;
        BlsObject visit(AstNode::Statement::Expression& ast) override; 
        BlsObject visit(AstNode::Statement::If &ast) override; 
        BlsObject visit(AstNode::Statement::For &ast) override; 
        BlsObject visit(AstNode::Statement::While &ast) override; 
        BlsObject visit(AstNode::Expression::Binary &ast) override;
        BlsObject visit(AstNode::Expression::Function &ast) override; 
        

        // Final Product
        std::unordered_map<ControllerID, ControllerSource>& getControllerSplit(){
            return this->ctlSourceMap; 
        } 
    
    


    }; 

}
