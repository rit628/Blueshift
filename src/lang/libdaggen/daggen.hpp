#pragma once
#include "ast.hpp"
#include "bindmapper.hpp"
#include "visitor.hpp"
#include <unordered_map>


namespace BlsLang{
    class DagGen : public Visitor{

       

        private: 
            // Data structures
            std::unordered_map<task_name_t, std::unordered_map<parameter_name_t, DeviceCoords>> param_map; 
                
        private: 
            // load the task names: 
            
        

        public: 
            // load the task names: 
            void load_bind_data(std::unordered_map<task_name_t, std::unordered_map<parameter_name_t, DeviceCoords>> &coords); 
           
            
            #define AST_NODE(Node, ...) \
            BlsObject visit(Node& ast) override;
            #include "include/NODE_TYPES.LIST"
            #undef AST_NODE
            

            
        public:
            // Show the binding map
            void DEBUG_show_map();

    }; 
}
