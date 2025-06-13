#pragma once
#include "ast.hpp"
#include "include/Common.hpp"
#include "../libsymgraph/symgraph.hpp"
#include <unordered_map>



namespace BlsLang{
    class Divider : public Printer{

    private: 
        std::unordered_map<ControllerID, AstNode::Source> CodeMap;  
        std::unordered_set<ControllerID> AstNode; 

    public: 
        Divider():Printer(std::cout) {}

        BlsObject visit(AstNode::Source& ast) override; 
        BlsObject visit(AstNode::Function::Oblock& ast) override; 
        BlsObject visit(AstNode::Statement::Declaration& ast) override;
        BlsObject visit(AstNode::Statement::Expression& ast) override; 
        BlsObject visit(AstNode::Statement::If &ast) override; 
        
        BlsObject visit(AstNode::Expression::Access& ast) override; 
        BlsObject visit(AstNode::Expression::Binary &ast) override; 



    }; 

}
