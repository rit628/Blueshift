#include "bindmapper.hpp"
#include "ast.hpp"
#include "bls_types.hpp"
#include <iostream>
#include <unordered_map>
#include <variant>

using namespace BlsLang; 


std::string BindMapper::extract_ctl_name(std::string &binding_string){
    int div_idx = binding_string.find(':'); 
    return binding_string.substr(0, div_idx); 
}

void BindMapper::DEBUG_show_map(){
    for(auto& obj : this->sym_map){
        std::cout<<"Task: "<<obj.first<<std::endl; 
        for(auto& obj : obj.second){
            std::cout<<"Param: "<<obj.first<<" -> ";
            std::cout<<"Device Name "<<obj.second.device_bind<<" on "<<obj.second.controller_name<<std::endl; 

        }
        std::cout<<"---------------------------------------"<<std::endl; 
    }
}


std::unordered_map<task_name_t, std::unordered_map<parameter_name_t, DeviceCoords>> BindMapper::get_map(){
    return this->sym_map; 
}


/*
    Visitors 
*/


BlsObject BindMapper::visit(AstNode::Source& ast) {   
    // Visit the task and setup functions
    in_setup = true; 
    for(auto& tsk : ast.tasks){
        tsk->accept(*this); 
    }
    ast.setup->accept(*this); 
    in_setup = false; 

    return true; 
}

BlsObject BindMapper::visit(AstNode::Setup& ast) {
    for(auto& stmt : ast.statements){
        stmt->accept(*this); 
    }
   return true; 
}

BlsObject BindMapper::visit(AstNode::Function::Task& ast){
    if(in_setup){
        // Populate the parameter list: 
        std::string& task_name = ast.name; 
        std::unordered_map<parameter_name_t, DeviceCoords> pdev_map; 
        this->task_params.emplace(task_name, ast.parameters); 
        for(auto& str : ast.parameters){    
            pdev_map.emplace(str, DeviceCoords{});
        }
        this->sym_map.emplace(task_name, pdev_map);
        return true; 
    }

    return true; 
}

BlsObject BindMapper::visit(AstNode::Statement::Declaration& ast) {
    if(in_setup){
        std::string dev_name = ast.name; 
        std::string ctl_name; 

        if(ast.value.has_value()){
            auto val = ast.value->get()->accept(*this); 
            auto binding_string = std::get<BlsType>(val); 
            if(std::holds_alternative<std::string>(binding_string)){
                ctl_name = this->extract_ctl_name(std::get<std::string>(binding_string)); 
            }
            this->device_coord_map.emplace(dev_name, DeviceCoords(dev_name, ctl_name)); 
        }
        return true; 
    }


    return true; 
}


BlsObject BindMapper::visit(AstNode::Statement::Expression &ast){
    ast.expression->accept(*this); 

    return true; 
}

BlsObject BindMapper::visit(AstNode::Expression::Function& ast) {
    if(in_setup){
        std::string task_name = ast.name; 
        auto& param_names = this->task_params.at(task_name);
        auto& param_real_map = this->sym_map.at(task_name); 
        
        for(int i = 0 ; i < ast.arguments.size(); i++){
            // Get the object string name
            auto& expr = ast.arguments.at(i); 
            auto type = std::get<BlsType>(expr->accept(*this)); 
            std::string device_name = std::get<std::string>(type); 

            // Get the param map; 
            auto& coord = this->device_coord_map.at(device_name); 
            std::string param_name = param_names.at(i); 
            param_real_map.at(param_name) = coord; 
        }

        return true; 
    }

    return true; 
}

BlsObject BindMapper::visit(AstNode::Expression::Access& ast) {     
    if(in_setup){
        return ast.object;
    }
    return true; 
}


BlsObject BindMapper::visit(AstNode::Expression::Literal& ast) {
    if(in_setup){
        auto& lit_variant = ast.literal; 
        if(std::holds_alternative<std::string>(lit_variant)){
            return std::get<std::string>(lit_variant); 
        }
    }
    return true; 
}


