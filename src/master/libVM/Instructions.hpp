#pragma once 


#include <variant> 
#include <vector> 
#include <unordered_map> 
#include <string> 
#include "HeapDescriptors.hpp"


enum class OPKIND{
    STACK, 
    HEAP, 
    BINARY, 
    CONTROL, 
    TRAP, 
    PREPROCESSING
};


enum class OPCODE{
    // Stack Manipulation Commands
    PUSH, 
    STORE,
    LOAD, 
    MK_SCOPE, 
    END_SCOPE, 
    
    // Unary Operators: 
    NOT, 

    // Binary Operator Functions: 
    AND, 
    OR,

    GT, 
    GTE, 
    LT,
    LTE, 

    ADD,
    SUB, 
    MULT, 
    DIV, 
    MOD, 

    EQU,

    // Control Statements 
    B_COND, 
    JMP, 
    END, 

    // The end loop resets the 
    OBLOCK_LOOP, 

    // Heap Manipulation Controls
    MKHEAP, 
    ACC, 
    EMPLACE, 
    APPEND, 
    SIZE, 

    // Trap Controls: 
    PRINT, 
    PRINTLN,

    // Pre processing (not used in VM): 
    BREAKPOINT,
    STORE_DECL

}; 


struct Instruction{
    OPKIND kind; 
    OPCODE code; 
    Prim arg1; 
    int arg2; 
}; 