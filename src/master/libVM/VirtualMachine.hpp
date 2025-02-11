#include <stack> 
#include "HeapDescriptors.hpp"
#include "Instructions.hpp"
#include "../libCommon/Common.hpp"

using in_StateMap = std::unordered_map<std::string, std::deque<std::shared_ptr<HeapDescriptor>>>;
using in_RefereshMap = std::unordered_map<std::string, int>; 

// Replace this with the thread safe queue that colin made (add the ticker stuff later...)!
using out_tsQueue = std::deque<std::shared_ptr<HeapDescriptor>>; 

class VM{
    private: 

        //  VM_stack, scope_stack, bytecode
        std::vector<Prim> vm_stack; 
        std::vector<int> scope_stack; 
        std::vector<Instruction> bytecode;
        std::vector<std::shared_ptr<HeapDescriptor>>  outputStates; 

        // Variable count: 
        int inputCount; 


        /*
        // State transfer items: 
        in_StateMap &state_queue;
        in_RefereshMap &refresh_map;  

        // Output State queue (add output ticker line as well)
        out_tsQueue &outQueue; 
        */

        std::string name; 
        bool running; 

    
        /*
            Oblock offset for the code.

            Once the code offset is the bytecode instruction where the oblock starts. 
            Once the oblock ends, the OBLOCK_LOOP instruction is reached and the ip 
            is set to code_offset; 
        */

        int code_offset = 0; 
        
        // Instruction ptr & stack ptr 
        std::vector<Instruction>::iterator ip; 
        std::vector<Prim>::iterator sp; 

        // Binary Operator: 
        Prim BinaryHandler(OPCODE code, Prim lhs, Prim rhs); 

        // Action dispatcher
        void action(Instruction &instr); 

        // Kind Specific actions: 
        void actionStack(Instruction &instr); 
        void actionHeap(Instruction &instr); 
        void actionControl(Instruction &instr); 
        void actionBinary(Instruction &instr); 
        void actionTrap(Instruction &instr); 

    public: 

        VM(std::string name, std::vector<Instruction> &bytecode, int bc_offset);
        void transform(std::vector<std::shared_ptr<HeapDescriptor>> &input, std::vector<std::shared_ptr<HeapDescriptor>> &output); 
}; 