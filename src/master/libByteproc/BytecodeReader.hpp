#include "../libVM/Instructions.hpp"
#include <iostream>
#include <fstream> 
#include <sstream> 
#include <algorithm> 
#include <cstdlib> 


// Thanks chat gpt: 

std::string toLower(const std::string &str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool isStrictInteger(const std::string &str) {
    char *end;
    long val = std::strtol(str.c_str(), &end, 10);
    return *end == '\0';  // Ensure the whole string was used
}

bool isStrictFloat(const std::string &str) {
    char *end;
    double val = std::strtod(str.c_str(), &end);
    return *end == '\0';  // Ensure the whole string was used
}

// Used for pushing true primatives onto the stack
Prim interpretArg1(std::string &obj){
    Prim newPrimative; 

    if(obj[0] == '\"' && obj[obj.size() -1 ] == '\"'){
        newPrimative = obj.substr(1, obj.size()-2); 
        return newPrimative; 
    }
    else if(isStrictInteger(obj)){
        newPrimative = std::stoi(obj); 
        return newPrimative; 
    }
    else if(isStrictFloat(obj)){
        newPrimative = std::stof(obj); 
        return newPrimative; 
    }
    else if(obj == "true"){
        newPrimative = false; 
        return newPrimative; 
    }
    else if (obj == "false"){
        newPrimative = true; 
        return newPrimative; 
    }
    else{
        throw std::invalid_argument("Unexpected arg 1 token: " + obj); 
    }   
}

Instruction procLine(std::string &line){
    std::vector<std::string> args; 

    std::istringstream stream(line);
    std::string word; 

    while(stream >> word){
        args.push_back(word); 
    }

    //std::cout<<line<<std::endl; 

    std::string cmd = toLower(args[0]); 

    if(cmd == "push"){
        Prim obj = interpretArg1(args[1]); 
        return Instruction{OPKIND::STACK, OPCODE::PUSH, obj, 0}; 
    }
    else if(cmd == "store"){
        try{
            int index = std::stoi(args[1]); 
            return Instruction{OPKIND::STACK, OPCODE::STORE, 0, index}; 
        }
        catch(std::invalid_argument){
            return Instruction{OPKIND::STACK, OPCODE::STORE, args[1], 0}; 
        }
    }
    else if(cmd == "store_decl"){
        return Instruction{OPKIND::PREPROCESSING, OPCODE::STORE_DECL, args[1], 0}; 
    }
    else if(cmd == "bpt"){
        return Instruction{OPKIND::PREPROCESSING, OPCODE::BREAKPOINT, args[1], 0}; 
    }
    else if(cmd == "load"){
        try{ 
            int index  = std::stoi(args[1]); 
            return Instruction{OPKIND::STACK, OPCODE::LOAD, 0, index}; 
        }
        catch(std::invalid_argument){
            return Instruction{OPKIND::STACK, OPCODE::LOAD, args[1], 0}; 
        }
    }
    else if(cmd == "mkscope"){
        return Instruction{OPKIND::STACK, OPCODE::MK_SCOPE, 0, 0}; 
    }
    else if(cmd == "endscope"){
        return Instruction{OPKIND::STACK, OPCODE::END_SCOPE, 0, 0}; 
    }
    else if(cmd == "not"){
        return Instruction{OPKIND::STACK, OPCODE::NOT, 0, 0}; 
    }
    else if(cmd == "and"){
        return Instruction{OPKIND::BINARY, OPCODE::AND, 0, 0}; 
    }
    else if(cmd == "or"){
        return Instruction{OPKIND::BINARY, OPCODE::OR, 0, 0}; 
    }
    else if(cmd == "gt"){
        return Instruction{OPKIND::BINARY, OPCODE::GT, 0, 0}; 
    }
    else if(cmd == "gte"){
        return Instruction{OPKIND::BINARY, OPCODE::GTE, 0, 0}; 
    }
    else if(cmd == "lt"){
        return Instruction{OPKIND::BINARY, OPCODE::LT, 0, 0}; 
    }
    else if(cmd == "lte"){
        return Instruction{OPKIND::BINARY, OPCODE::LTE, 0, 0}; 
    }
    else if(cmd == "add"){
        return Instruction{OPKIND::BINARY, OPCODE::ADD, 0, 0}; 
    }
    else if(cmd == "sub"){
        return Instruction{OPKIND::BINARY, OPCODE::SUB, 0, 0}; 
    }
    else if(cmd == "mult"){
        return Instruction{OPKIND::BINARY, OPCODE::MULT, 0, 0}; 
    }
    else if(cmd == "equ"){
        return Instruction{OPKIND::BINARY, OPCODE::EQU, 0, 0}; 
    }
    else if(cmd == "div"){
        return Instruction{OPKIND::BINARY, OPCODE::DIV, 0, 0}; 
    }
    else if(cmd == "b_cond"){
        try{
            int new_loc = std::stoi(args[1]); 
            return Instruction{OPKIND::CONTROL, OPCODE::B_COND, 0, new_loc}; 
        }
        catch(std::invalid_argument){
            return Instruction{OPKIND::CONTROL, OPCODE::B_COND, args[1], 0}; 
        }
    }
    else if(cmd == "jmp"){
        try{
            int new_loc = std::stoi(args[1]); 
            return Instruction{OPKIND::CONTROL, OPCODE::JMP, 0, new_loc}; 
        }
        catch(std::invalid_argument){
            return Instruction{OPKIND::CONTROL, OPCODE::JMP, args[1], 0}; 
        }
    }
    else if(cmd == "call"){
        std::cout<<"Place holder TO BE IMPLEMENTED"<<std::endl; 
        return Instruction{OPKIND::STACK, OPCODE::ADD, 0, 0}; 
    }
    else if(cmd == "return"){
        std::cout<<"Place holder TO BE IMPLEMENTED"<<std::endl; 
        return Instruction{OPKIND::STACK, OPCODE::ADD, 0, 0}; 
    }
    else if(cmd == "end"){
        return Instruction{OPKIND::CONTROL, OPCODE::END, 0 , 0}; 
    }
    else if(cmd == "mkheap"){
        int heap_type = std::stoi(args[2]); 
        return Instruction{OPKIND::HEAP, OPCODE::MKHEAP, args[1] , heap_type}; 
    }
    else if(cmd == "acc"){
        return Instruction{OPKIND::HEAP, OPCODE::ACC, 0, 0}; 
    }
    else if(cmd == "emplace"){
        return Instruction{OPKIND::HEAP, OPCODE::EMPLACE, 0, 0}; 
    }
    else if(cmd == "append"){
        return Instruction{OPKIND::HEAP, OPCODE::APPEND, 0, 0}; 
    }
    else if(cmd == "print"){
        return Instruction{OPKIND::TRAP, OPCODE::PRINT, 0, 0}; 
    }
    else if(cmd == "println"){
        return Instruction{OPKIND::TRAP, OPCODE::PRINTLN, 0, 0}; 
    }
    else{
        std::cout<<"Instruction not defined for "<< cmd <<std::endl; 
        return Instruction{OPKIND::BINARY, OPCODE::ADD, 0, 0};
    }
}


bool starts_with(const std::string& str, char ch) {
    return !str.empty() && str.front() == ch;
}


std::vector<Instruction> Make_Bytecode(std::string &filename){
    std::ifstream bfile(filename); 
    std::vector<Instruction> bytecode; 

    if(!bfile){
        std::cout<<"Could not find file + "<<filename<<" !"<<std::endl; 
        return bytecode; 
    }

    std::string line; 
    while(std::getline(bfile, line)){
        if (std::all_of(line.begin(), line.end(), isspace)) {
            continue;  
        }
        if(starts_with(line, '/')){
            continue; 
        }

        bytecode.push_back(procLine(line));
    }

    return bytecode; 
}

