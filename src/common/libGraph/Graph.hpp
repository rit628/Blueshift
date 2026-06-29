#include <deque>
#include <memory>
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
        PHI, 

        COND_JUNC, 
        COND_JUNC_ELIF, 
        COND_JUNC_ELSE, 

        PUSH, 
        PULL, 
        FOR,
        ITER_PHI,  
        DEVICE_WRITE, 
        SUBSCRIPT,  
        MEMBER, 
        LITERAL
    }; 

    struct PhiValue{
        SymbolID_t condition = 0; 
        SymbolID_t true_value = 0; 
        SymbolID_t false_value = 0; 
    }; 

    struct ForData{
        SymbolID_t init_symbol = 0; 
        SymbolID_t incrementor = 0; 
        SymbolID_t cond_expr = 0;
    };

    struct IterPhiValue{
        SymbolID_t assignment_symbol = 0; 
        SymbolID_t loop_symbol = 0; 
    }; 

    struct SubscriptValue{
        SymbolID_t root_id = 0; 
        SymbolID_t index_id = 0; 
    }; 

    struct OrderedBinaryValue{
        SymbolID_t lhs_id = 0; 
        SymbolID_t rhs_id = 0; 
    }; 

    struct SymbolDescriptor{
        public: 
            SymbolID_t id = 0; 
            std::string symbol_name;
            std::string root_symbol_name;  
            EXPR_TYPE expr_type; 
            TaskID_t task_id = 0; 
         
            TypeContainer types; 
            CtlID_t host_ctl = 0; 

            size_t bytecode_start = 0; 
            size_t bytecode_end = 0; 
            int stack_depth = -1; 
            
            std::vector<SymbolID_t> symbolic_depend;    
            std::vector<SymbolID_t> symbolic_fwd; 

            // CONDITIONAL INFORMATION NODES
            SymbolID_t conditional_symbol = 0; 
            std::vector<SymbolID_t> true_path;   
            std::unordered_map<std::string, SymbolID_t> value_under_cond; 
                        
            // ASSIGNEMENT/VALUE INFORMATION NODES
            SymbolID_t value_node = 0; 

            // PHI_CONDITION 
            PhiValue phi_value; 

            // FOR LOOP: 
            ForData for_value; 

            // ITER Value
            IterPhiValue iter_phi; 

            // Subscript Values
            SubscriptValue subscript_val; 

            // Ordered Binary 
            OrderedBinaryValue binary_val; 


            SymbolDescriptor(std::string &string){
                this->symbol_name = string; 
            }

            SymbolDescriptor(SymbolID_t sym_cnt, size_t bc_start, size_t bc_end, EXPR_TYPE type){
                this->expr_type = type; 
                this->bytecode_start = bc_start; 
                this->bytecode_end = bc_end; 
                this->id = sym_cnt; 
            }   
            

            void set_root_symbol_name(const std::string &nm){
                this->root_symbol_name = nm; 
            }
            
            void set_phi_params(SymbolID_t cond_sym, SymbolID_t true_sym, SymbolID_t false_sym){
                this->phi_value.false_value = false_sym; 
                this->phi_value.true_value = true_sym; 
                this->phi_value.condition = cond_sym; 
            }

            void set_iter_phi(SymbolID_t loop_symbol, SymbolID_t assignment_symbol){
                this->iter_phi.assignment_symbol = assignment_symbol;  
                this->iter_phi.loop_symbol = loop_symbol; 
            }


            void set_name(const std::string &nm){
                this->symbol_name = nm; 
            }

            void set_type(TypeContainer tc){
                this->types = tc; 
            }

            void add_true_path(SymbolID_t sym_id){
                this->true_path.push_back(sym_id); 

            }

            SymbolDescriptor() = default; 
    }; 


    /*
        Consolidation structs
    */

   
    

    class DAG{

        struct CondValue{
            SymbolID_t condition; 
            SymbolID_t value; 
        }; 

        struct CondData{   
            std::deque<CondValue> conditional_stack; 
            SymbolID_t alternative = 0; 
        }; 

        struct VariableContainer{
            std::unordered_map<std::string, int> sym_ref_cnt; 
            std::unordered_map<std::string, SymbolID_t> named_descs;
        }; 

        private: 
            
            std::unordered_map<std::string, SymbolID_t> declared_devices; 
            std::vector<VariableContainer> variable_stk; 
          
            std::map<SymbolID_t, SymbolDescriptor> descriptor_map;
            std::stack<SymbolID_t> symbol_desc;
            std::stack<SymbolID_t> conditional_dep_stack; 
            std::stack<std::vector<std::string>> affected_variables; 

            std::vector<SymbolID_t> task_initators;  
            std::vector<SymbolID_t> dev_writes; 

            int curr_stack_depth = 0; 

        private: 

            // DAG FIRST LEVEL FORMATION: 
            std::pair<std::string, int> get_last_ident(const std::string& pure_name); 
            
            void reassign_variable(const std::string& pure_name, SymbolDescriptor& sdesc); 
            void add_variable(std::string pure_name, int val, EXPR_TYPE expr); 
            TypeContainer extract_type_help(TypeContainer root_tc, std::deque<std::string>& field_names); 
            TypeContainer extract_type(TypeContainer root_tc, std::string &sub_types); 
            std::pair<std::string, std::string> get_access_pair(const std::string &sub_object); 


            VariableContainer& get_last_assign(const std::string &pure_name); 
            
            void push_conditional_dep(SymbolID_t cond_dep); 
            void pop_conditional_dep(); 

            void link_edge(SymbolID_t src, SymbolID_t dest); 
            void link_conditional_edge(SymbolID_t parent, SymbolID_t); 
            void unlink_edge(SymbolID_t src, SymbolID_t dest); 
    
            void inherit_bcrange(SymbolID_t dep, SymbolID_t child); 
            bool is_conditional(std::string &op); 
            TypeContainer get_binary_type(SymbolDescriptor& lhs, SymbolDescriptor &rhs, std::string& op); 

            // Consolidate Phi Data for Branches
            int construct_phi_node(const std::string& name, const CondValue& cv ,SymbolID_t false_value, size_t bcode_start, size_t bcode_end, SymbolID_t& count); 
            void coalesce_phi_nodes(std::unordered_map<std::string, CondData>& data_node, SymbolID_t &sym_count); 

            
        

        public: 
            int add_symbol(SymbolDescriptor &sd); 
            void push_symbol_frame(); 
            void pop_symbol_frame(); 
            void start_if_statement(); 
            void start_for_statement(std::array<bool, 3>& set_fields); 
            
            void complete_binary_statement(); 
            void complete_declare_statement(); 
            void complete_access_statement();  
            void complete_member_statement(); 
            void complete_subscript_statement(); 
            void complete_if_statement(SymbolID_t &sym_count); 
            void complete_for_statement(SymbolID_t &sym_count); 


            void print_type_helper(const TypeContainer &tc, const std::string &name, int tab_cnt);
            void DEBUG_print_symbol(const SymbolDescriptor &sdesc); 
            void DEBUG_print_descriptor_map(); 
            std::string DEBUG_print_sym_type(EXPR_TYPE type); 
            void DEBUG_print_type(const TypeContainer& tc); 
            void DEBUG_print_declared_map();      
    }; 



    



}





