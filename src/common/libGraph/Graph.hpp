#include <stdint.h>
#include <unordered_map>
#include <map>
#include <vector> 
#include "Serialization.hpp"
#include <set> 
#include <unordered_set>

namespace SymbolGraph{
    using SymbolName_t = std::string; 


    enum class EXPR_TYPE{
        ACCESS, 
        ASSIGNMENT, 
        DEVICE_ACCESS,
        DEVICE_DECLARATION, 
        DECLARATION,  
        TRAP_CALL, 
        BINARY_COM,
        BINARY_NO_COM,

        COND_JUNC, 
        COND_JUNC_ELIF, 
        COND_JUNC_ELSE, 

        PUSH, 
        PULL, 
        DEVICE_WRITE, 
        SUBSCRIPT,  
        LITERAL
    }; 

    struct ConditionalValue{
        std::vector<SymbolID_t> item; 
        SymbolID_t default_symbol; 

    }; 


    struct SymbolDescriptor{
        public: 
            SymbolID_t id = 0; 
            std::string symbol_name; 
            EXPR_TYPE expr_type; 
            TaskID_t task_id = 0; 
         
            TypeContainer types; 
            CtlID_t host_ctl = 0; 

            size_t bytecode_start = 0; 
            size_t bytecode_end = 0; 
            
            // Ingress nodes: 
            std::vector<SymbolID_t> symbolic_depend;     
            std::vector<SymbolID_t> symbolic_fwd; 
            
            // The default option is chosen if the alternative
            std::vector<SymbolID_t> cond_dependent; 
     
    
            SymbolDescriptor(std::string &string){
                this->symbol_name = string; 
            }

            SymbolDescriptor(int sym_cnt, size_t bc_start, size_t bc_end, EXPR_TYPE type){
                this->expr_type = type; 
                this->bytecode_start = bc_start; 
                this->bytecode_end = bc_end; 
                this->id = sym_cnt; 
            }   

            void set_name(const std::string &nm){
                this->symbol_name = nm; 
            }

            void set_type(TypeContainer tc){
                this->types = tc; 
            }

            SymbolDescriptor() = default; 
    }; 




    class DAG{

        struct VariableContainer{
            std::unordered_map<std::string, int> sym_ref_cnt; 
            std::unordered_map<std::string, SymbolID_t> named_descs;
        }; 

        private: 

            std::unordered_map<std::string, SymbolDescriptor> declared_devices; 
            std::vector<VariableContainer> variable_stk; 
          
            std::map<SymbolID_t, SymbolDescriptor> descriptor_map;
            std::stack<SymbolID_t> symbol_desc;
            std::stack<SymbolID_t> conditional_dep_stack; 

            std::vector<SymbolID_t> task_initators;  
            std::vector<SymbolID_t> dev_writes; 

        private: 

            // DAG FIRST LEVEL FORMATION: 
            std::pair<std::string, int> get_last_ident(std::string& pure_name); 
            void add_variable(std::string &pure_name, int val, EXPR_TYPE expr); 
        
            void push_conditional_dep(SymbolID_t cond_dep); 
            void pop_conditional_dep(); 

            void link_edge(SymbolID_t src, SymbolID_t dest); 
            void unlink_edge(SymbolID_t src, SymbolID_t dest); 


            void inherit_bcrange(SymbolID_t dep, SymbolID_t child); 
            bool is_conditional(std::string &op); 
            TypeContainer get_binary_type(SymbolDescriptor& lhs, SymbolDescriptor &rhs, std::string& op); 

            // BRANCH ANALYSIS AND PHI FUNCTION FORMATION: 
            void finalize_branch(); 
            void finalize_access(); 

        public: 
            void add_symbol(SymbolDescriptor &sd); 
            void push_symbol_frame(); 
            void pop_symbol_frame(); 
            
            void complete_binary_statement(); 
            void complete_declare_statement(); 
            void complete_access_statement();  
            void complete_if_statement(); 
        
            void print_type_helper(const TypeContainer &tc, const std::string &name, int tab_cnt);
            void DEBUG_print_symbol(const SymbolDescriptor &sdesc); 
            void DEBUG_print_descriptor_map(); 
            std::string DEBUG_print_sym_type(EXPR_TYPE type); 
            void DEBUG_print_type(const TypeContainer& tc); 
            void DEBUG_print_declared_map();      
    }; 



    



}





