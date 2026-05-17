#pragma once
#include "ast.hpp"
#include "visitor.hpp"
#include <string>
#include <unordered_map>
#include <vector>

using task_name_t = std::string; 
using parameter_name_t = std::string; 
using device_name_t = std::string;
using controller_name_t = std::string; 


namespace BlsLang{
    struct DeviceCoords{
        device_name_t device_bind; 
        controller_name_t controller_name; 

        DeviceCoords(std::string& dev_name, std::string&  ctl_name){
            device_bind = dev_name; 
            controller_name = ctl_name; 
        }

        DeviceCoords(){
            device_bind = ""; 
            controller_name = ""; 
        }
    }; 


    class BindMapper : public Visitor{

        private: 
            bool in_setup = false; 
            std::unordered_map<device_name_t, DeviceCoords> device_coord_map; 
            std::unordered_map<task_name_t, std::unordered_map<parameter_name_t, DeviceCoords>> sym_map; 
            std::unordered_map<task_name_t, std::vector<parameter_name_t>> task_params; 
                
        private: 
            std::string extract_ctl_name(std::string& binding_string); 
            void DEBUG_show_map(); 


        public: 
            BlsObject visit(AstNode::Source& node) override;
            BlsObject visit(AstNode::Setup& ast) override; 
            BlsObject visit(AstNode::Function::Task& ast) override;   
            BlsObject visit(AstNode::Statement::Declaration& ast) override; 
            BlsObject visit(AstNode::Statement::Expression &ast) override; 
            BlsObject visit(AstNode::Expression::Function& ast) override; 
            BlsObject visit(AstNode::Expression::Access& ast) override; 
            BlsObject visit(AstNode::Expression::Literal& ast)  override; 

        public: 
            std::unordered_map<task_name_t, std::unordered_map<parameter_name_t, DeviceCoords>> get_map(); 
    }; 
}