#include "optimizer.hpp"
#include "ast.hpp"
#include <any>

std::any Optimizer::visit(AstNode::Source &source) {
    source.getSetup()->accept(*this);
}

std::any Optimizer::visit(AstNode::Setup &setup){
    auto& stateVec = setup.getStatements(); 
    for(auto& statement : stateVec){
        statement->accept(*this); 
    }
} 

std::any Optimizer::visit(AstNode::Statement::Declaration &statement){
    auto &name = statement.getName(); 
    std::cout<<"Parent Object: "<<name<<std::endl; 
    auto &type = statement.getType(); 
    auto &value = statement.getValue(); 

}

std::any Optimizer::visit(AstNode::Expression::Binary &binExpr){
    auto op = binExpr.getOp(); 

    std::cout<<"Left Object"<<std::endl; 
    binExpr.getLeft()->accept(*this);

    std::cout<<"Right Object"<<std::endl;
    binExpr.getRight()->accept(*this);
}

std::any Optimizer::visit(AstNode::Expression::Access &access){
    std::cout<<"Object Found:  "<<access.getObject()<<std::endl; 
}