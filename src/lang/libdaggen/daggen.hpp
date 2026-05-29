#pragma once
#include "ast.hpp"
#include "bindmapper.hpp"
#include "bls_types.hpp"
#include "reserved_tokens.hpp"
#include "visitor.hpp"
#include <unordered_map>
#include "Serialization.hpp"
#include <stack> 
#include <iostream>
#include "Graph.hpp"

namespace BlsLang{
    using devtype_name_t = std::string; 

    class DagGen : public Visitor{

        private: 
   
            struct DeviceType{
                std::string root_type; 
                std::unordered_map<std::string, TypeContainer> params; 
            }; 
            
            // Things specific to a certain tassk
            struct TaskContext{
                bool eval_params = false; 
                task_name_t task_name;  
                std::unordered_set<std::string> param_names; 
               
                SymbolGraph::DAG dag; 
                std::unordered_map<std::string, int> name_to_id; 

                // Stack 
                std::stack<TypeContainer> type_stk; 
            }; 

            // Data structures
            TaskContext curr_task_ctx; 
            std::unordered_map<task_name_t, TaskContext> task_to_ctx; 
            std::unordered_map<std::string, DeviceType> device_map; 
            int counter = 0; 
    

        private: 
            

            void assign_score(TypeContainer &tc); 
            TypeContainer parse_template_type(std::string &type_name); 
            std::string get_symbol_count(std::string &raw_symbol_name); 
            void increment_symbol_refcount(std::string& raw_symbol_name); 
            void clear_task_context(); 
            TypeContainer produce_type_data(std::string &type_name); 
            void build_type_container(std::string &type_str);
            SymbolGraph::EXPR_TYPE get_bin_expr_type(std::string &bin_op);  

            
            void print_type_helper(TypeContainer &tc, const std::string &name, int tab_cnt); 
            void DEBUG_print_type(TypeContainer &tc); 
            void DEBUG_print_type_stk(); 
        
        public: 
            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
        
        public:
            // Show the binding map
            void load_device_params(); 


    }; 
}
