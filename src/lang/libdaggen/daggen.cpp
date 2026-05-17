#include "daggen.hpp"
#include "ast.hpp"

using namespace BlsLang; 


void DagGen::DEBUG_show_map(){
    for(auto& obj : this->param_map){
        std::cout<<"Task: "<<obj.first<<std::endl; 
        for(auto& obj : obj.second){
            std::cout<<"Param: "<<obj.first<<" -> ";
            std::cout<<"Device Name "<<obj.second.device_bind<<" on "<<obj.second.controller_name<<std::endl; 

        }
        std::cout<<"---------------------------------------"<<std::endl; 
    }
}

void DagGen::load_bind_data(std::unordered_map<task_name_t, std::unordered_map<parameter_name_t, DeviceCoords>> &coords){
    this->param_map = coords; 
}

/*
VISITOR FUNCTIONS
*/

BlsObject DagGen::visit(AstNode::Source& ast) {
    std::cout<<"Source node"<<std::endl; 
    for(auto& tsk : ast.tasks){
        tsk->accept(*this); 
    }
    return true; 
}

BlsObject DagGen::visit(AstNode::Setup& ast) {
    std::cout<<"Setup Node"<<std::endl; 
   return true; 
}

BlsObject DagGen::visit(AstNode::Function::Task& ast){
    std::cout<<"Task Function Node"<<std::endl; 
    for(auto& stmt : ast.statements){
        stmt->accept(*this); 
    }
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Declaration& ast) {
    std::cout<<"Statement Declaration Node"<<std::endl; 
    

    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Binary &ast){
    std::cout<<"Expression Binary Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Statement::Expression &ast){
    std::cout<<"Statement Expression Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Function& ast) {
   std::cout<<"Expression Function Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Access& ast) {
    std::cout<<"Expression Access Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Literal& ast) {
    std::cout<<"Expresion Literal Node"<<std::endl; 
  
   
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::If& ast) {
    std::cout<<"Statement If Node"<<std::endl; 
  
    return true; 
    
}

BlsObject DagGen::visit(AstNode::Statement::For& ast) {
    std::cout<<"Statement For Node"<<std::endl; 

  
    return true; 
    
}

BlsObject DagGen::visit(AstNode::Statement::While& ast) {
    std::cout<<"Statement While Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Group& ast) {
    std::cout<<"Expression Group Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Unary& ast){
   std::cout<<"Expression Unary Node"<<std::endl; 


    return true; 
}



BlsObject DagGen::visit(AstNode::Function::Procedure& ast) {
    std::cout<<"Function Procedure Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Statement::Return& ast) {
    std::cout<<"Statement Return Node"<<std::endl; 


 
    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Continue& ast) {
    std::cout<<"Statement Continue Node"<<std::endl;  


    return true; 
}

BlsObject DagGen::visit(AstNode::Statement::Break& ast) {
    std::cout<<"Statement Break Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::Method& ast) {
    std::cout<<"Expression Method Node"<<std::endl; 


    return true; 
}


BlsObject DagGen::visit(AstNode::Expression::List& ast) {
    std::cout<<"Expression List Node"<<std::endl; 

    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Set& ast) {
    std::cout<<"Expression Set Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Expression::Map& ast) {
    std::cout<<"Expression Map Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Specifier::Type& ast) {
    std::cout<<"Specifier Type Node"<<std::endl; 


    return true; 
}

BlsObject DagGen::visit(AstNode::Initializer::Task &ast){
    std::cout<<"Specifier Type Node"<<std::endl; 
    


    return true; 
}








