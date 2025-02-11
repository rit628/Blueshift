#include <iostream> 
#include <variant> 
#include "Instructions.hpp"
#include <type_traits>


#define NUM_PRIM int, float, bool
using NumPrim = std::variant<NUM_PRIM>; 

#define BIN_EVAL(a, b, op)\
static_cast<commonType>(a) op static_cast<commonType>(b)

// Used to resolve primative in get objects: 
Prim resolvePrim(Prim &obj){
    if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(obj)){
        auto primHeap = std::get<std::shared_ptr<HeapDescriptor>>(obj); 
        if(auto primative = std::dynamic_pointer_cast<PrimDescriptor>(primHeap)){
            return primative->variant; 
        }
        else{
            return obj; 
        }
    }
    else{
        return obj;
    }
}

void BinEval(NumPrim left, NumPrim right, OPCODE code, Prim &ptype){
    std::visit([&](auto&& lhs, auto&& rhs) {
        using ltype = std::decay_t<decltype(lhs)>;
        using rtype = std::decay_t<decltype(rhs)>; 

        // Binary operations only supports arithemtic types: 

        if(std::is_arithmetic_v<ltype> && std::is_arithmetic_v<rtype>) {
            using commonType = std::common_type_t<ltype, rtype>; 
            switch(code){
                case OPCODE::ADD:
                    ptype = BIN_EVAL(lhs, rhs, +); 
                    break;
                case OPCODE::SUB:
                    ptype = BIN_EVAL(lhs, rhs, -); 
                    break; 
                case OPCODE::MULT:
                    ptype = BIN_EVAL(lhs, rhs, *); 
                    break;
                case OPCODE::DIV:
                    ptype = BIN_EVAL(lhs, rhs, /); 
                    break;
                case OPCODE::GT :
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, >)); 
                    break;
                case OPCODE::GTE :
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, >=)); 
                    break;
                case OPCODE::LT :
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, <)); 
                    break;
                case OPCODE::LTE :
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, <=)); 
                    break;
                case OPCODE::AND : 
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, &&)); 
                    break; 
                case OPCODE::OR: 
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, ||)); 
                    break;
                case OPCODE::EQU: 
                    ptype = static_cast<bool>(BIN_EVAL(lhs, rhs, ==)); 
                    break; 
                default : {
                    throw std::invalid_argument("Unknown binary operations"); 
                }

            }

        }

    }, left, right);   
}


NumPrim extractNumeric(Prim var){
    if(std::holds_alternative<int>(var)){
        return NumPrim(std::get<int>(var)); 
    }
    else if(std::holds_alternative<bool>(var)){
        return NumPrim(std::get<bool>(var)); 
    }
    else if(std::holds_alternative<float>(var)){
        return NumPrim(std::get<float>(var)); 
    }
    else{
        throw std::runtime_error("Cannot deduce a number from type"); 
        return NumPrim(0); 
    }
}


Prim opPrims(Prim lhs, Prim rhs, OPCODE op){
    auto n1 = extractNumeric(lhs); 
    auto n2 = extractNumeric(rhs); 

    Prim a = 0; 
    BinEval(n1, n2, op, a);    

    return a; 
}


