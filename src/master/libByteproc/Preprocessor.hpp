#include "../libVM/Instructions.hpp"
#include <deque>
#include <unordered_map> 
#include <vector> 

using SymbolTable = std::unordered_map<std::string, int>; 

class Preprocessor{
    private: 
        std::vector<SymbolTable> scopedSymbolTable;
        std::unordered_map<std::string, int> jmpTable;
        int varCount = 0; 

        std::pair<int, int>  getLatestReference(std::string &dream){
            int scope_loc = this->scopedSymbolTable.size() - 1; 

            int found_offset = -1; 
            bool found = false; 

            while(!found){
                auto curr_table = this->scopedSymbolTable[scope_loc]; 
                auto prospect = curr_table.find(dream); 
                if(prospect != curr_table.end()){
                    found_offset = prospect->second; 
                    found = true; 
                    break; 
                }

                scope_loc--; 
            }

            if(found_offset == -1){
                throw std::invalid_argument("Could not find symbol: " + dream); 
            }

            return std::make_pair(scope_loc, found_offset); 
        }

        // populate the jump table with breakpoint entries: 
        void jumpTableFill(std::vector<Instruction> &bytecode, int start_offset){
            auto vec_pos = bytecode.begin() + start_offset;
            int int_pos = start_offset; 

            while(vec_pos->code != OPCODE::END){
 
                if(vec_pos->code == OPCODE::BREAKPOINT) {
                    if(std::holds_alternative<std::string>(vec_pos->arg1)){
                        std::string break_symbol = std::get<std::string>(vec_pos->arg1); 
                        this->jmpTable[break_symbol] = int_pos; 
                    }
                    else{
                        throw std::invalid_argument("Non string symbol to create a breakpoint"); 
                    }

                }

                vec_pos++;
                int_pos++; 

            }


        }


    public: 
        Preprocessor() = default; 
        void Preprocess(std::vector<Instruction> &bytecode, int start_offset){
            // populate jump table
            jumpTableFill(bytecode, start_offset); 


            auto vec_pos = bytecode.begin() + start_offset;
            auto int_pos = start_offset; 
            while(vec_pos->code != OPCODE::END){
                switch(vec_pos->code){
                    case(OPCODE::MK_SCOPE) :{
                        SymbolTable st; 
                        scopedSymbolTable.push_back(st); 
                        break; 
                    }

                    case(OPCODE::END_SCOPE) : {
                        this->scopedSymbolTable.pop_back(); 
                        break; 
                    }

                    case(OPCODE::LOAD) : 
                    case(OPCODE::STORE) : {
                        if(std::holds_alternative<std::string>(vec_pos->arg1)){
                            std::string dream; 
                            dream = std::get<std::string>(vec_pos->arg1); 
                            auto von = getLatestReference(dream); 
                            vec_pos->arg1 = von.first; 
                            vec_pos->arg2 = von.second;
                        }

                        break; 
                    }

                    case(OPCODE::STORE_DECL) : {
                        std::string dream;  
                        if(std::holds_alternative<std::string>(vec_pos->arg1)){
                            dream = std::get<std::string>(vec_pos->arg1); 
                        }
                        else{
                            throw std::invalid_argument("Attempted to store a non string symbol"); 
                        }
                        
                        if(this->scopedSymbolTable.empty()){
                            throw std::runtime_error("No scope defined for object"); 
                        }

                        varCount = this->scopedSymbolTable.back().size();   

                        scopedSymbolTable.back()[dream] = varCount; 

                        int scope_level = this->scopedSymbolTable.size() - 1;    
                        int rel_index = this->scopedSymbolTable.back().size();              

                        vec_pos->kind = OPKIND::STACK; 
                        vec_pos->code = OPCODE::STORE; 
                        vec_pos->arg1 = scope_level; 
                        vec_pos->arg2 = varCount; 

                        break; 
            
                    }
                    
                    case(OPCODE::B_COND) :
                    case(OPCODE::JMP) : {
                        if(std::holds_alternative<std::string>(vec_pos->arg1)){
                            auto jumpTo = std::get<std::string>(vec_pos->arg1);
                            
                            if(this->jmpTable.find(jumpTo) == this->jmpTable.end()){
                                throw std::invalid_argument("Unknown Jump Breakpoint " + jumpTo); 
                            }

                            auto dest = this->jmpTable[jumpTo]; 

                            vec_pos->arg2 = dest - int_pos; 
                        }

                        break; 
                    }

                    default : {
                        break ;
                    }

                }

                int_pos++; 
                vec_pos++; 
            }
        }


}; 