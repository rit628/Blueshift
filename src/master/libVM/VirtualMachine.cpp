#include "VirtualMachine.hpp"
#include "VariantOperator.hpp"
#include "../libCommon/Common.hpp"


// VM Constructor object. Instantiates the name, in_code, cntrl, code offset
VM::VM(std::string name, std::vector<Instruction> &in_code, int offset)
{
    this->name = name; 
    this->bytecode = in_code; 
    this->code_offset = offset; 
    this->inputCount = 0; 
    

}

void VM::actionBinary(Instruction &instr){
    Prim right = this->vm_stack.back(); 
    this->vm_stack.pop_back(); 
    Prim left = this->vm_stack.back(); 
    this->vm_stack.pop_back(); 

    if(instr.code == OPCODE::MOD){
        int right_int = std::get<int>(right); 
        int left_int = std::get<int>(left); 

        this->vm_stack.push_back(left_int % right_int); 
    }


    Prim res = opPrims(resolvePrim(left), resolvePrim(right), instr.code); 
    this->vm_stack.push_back(res); 
}


void VM::actionStack(Instruction &instr){
    switch(instr.code){
        case(OPCODE::PUSH) : {
            this->vm_stack.push_back(instr.arg1); 
            break; 
        }

        // Used to general storage (assignment) 
        // Store Expansion
        case(OPCODE::STORE) : {
            // Use -1
            if(instr.arg2 == -1){
                Prim insert_state = this->vm_stack.back(); 
                this->vm_stack.pop_back();

                if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(this->vm_stack.back())){

                    auto& heapObj = std::get<std::shared_ptr<HeapDescriptor>>(this->vm_stack.back()); 

                    if(auto bob = std::dynamic_pointer_cast<PrimDescriptor>(heapObj)){

                        if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(insert_state)){
                            auto insertHeap = std::get<std::shared_ptr<HeapDescriptor>>(insert_state); 

                            if(auto primInsert = std::dynamic_pointer_cast<PrimDescriptor>(insertHeap)){
                                primInsert->variant = primInsert->variant; 
                            }
                            else{
                                std::cerr<<"Cannot set primative object to non-primative object"<<std::endl; 
                            }
                        }
                        else{
                            bob->variant = insert_state; 
                        }

                    }
                    else if(auto bob = std::dynamic_pointer_cast<VectorDescriptor>(heapObj)){
                        
                        if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(insert_state)){
                            auto insertHeap = std::get<std::shared_ptr<HeapDescriptor>>(insert_state); 

                            if(auto vecInsert = std::dynamic_pointer_cast<VectorDescriptor>(insertHeap)){
                                bob->vector = vecInsert->vector; 
                            }
                            else{
                                std::cerr<<"Cannot set vector object to non-vector object"<<std::endl; 
                            }
                        }
                        else{
                             std::cerr<<"Cannot set vector object to non-vector object"<<std::endl; 
                        }

                    }
                    else if(auto bob = std::dynamic_pointer_cast<MapDescriptor>(heapObj)){

                        if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(insert_state)){
                            auto insertHeap = std::get<std::shared_ptr<HeapDescriptor>>(insert_state); 

                            if(auto vecInsert = std::dynamic_pointer_cast<MapDescriptor>(insertHeap)){
                                bob->map = vecInsert->map; 
                            }
                            else{
                                std::cerr<<"Cannot set map object to non-map object"<<std::endl; 
                            }
                        }
                        else{
                             std::cerr<<"Cannot set map object to non-map object"<<std::endl; 
                        }

                    }

                }
                else{
                    std::cout<<"Warning: using heap store option to store write to heap-defined object?"<<std::endl; 
                }

                this->vm_stack.pop_back(); 

               
            }
            else{ 
                int scope_val = 0; 
                int scope_offset = 0;

                if(std::holds_alternative<int>(instr.arg1)){
                    scope_val = std::get<int>(instr.arg1); 
                }
                else{
                    throw std::runtime_error("Unidefined scoped identifier store"); 
                }


                if(this->scope_stack.size() < scope_val || this->scope_stack.empty()){
                    throw std::runtime_error("variable is being stored outside specified scope!"); 
                }
                else{
                    scope_offset = this->scope_stack[scope_val];  
                }

                int pos = instr.arg2 + scope_offset; 

                if(pos != this->vm_stack.size() - 1 ){   
                    Prim latest = resolvePrim(this->vm_stack.back()); 
                    this->vm_stack[pos] = latest; 
                    this->vm_stack.pop_back(); 
                }
                else{
                    this->vm_stack.back() = resolvePrim(this->vm_stack.back());
                }
            }

            break; 
        }
        case(OPCODE::LOAD) : {
            int scope_val = 0; 
            int scope_offset = 0;

            if(std::holds_alternative<int>(instr.arg1)){
                scope_val = std::get<int>(instr.arg1); 
            }
            else{
                throw std::runtime_error("Unidefined scoped identifier store"); 
            }

            if(this->scope_stack.size() < scope_val){
                throw std::runtime_error("Item is being used outside specified scope!"); 
            }
            else{
                scope_offset = this->scope_stack[scope_val];  
            }

            Prim targ = this->vm_stack[instr.arg2 + scope_offset]; 
            this->vm_stack.push_back(targ); 
            break; 
        }

        case(OPCODE::MK_SCOPE) : {
            // init scope, always set offset to 0 (to account for pre-existing states)
            if(this->scope_stack.empty()){
                this->scope_stack.push_back(0); 
            }
            else{
                this->scope_stack.push_back(this->vm_stack.size()); 
            }

            break; 
        }

        case(OPCODE::END_SCOPE) : { 
            if(!this->scope_stack.empty()){
                int new_sz = this->scope_stack.back(); 
                this->scope_stack.pop_back(); 
                // If stack is getting eradicated, then end of program, transfer states to output
                if(new_sz == 0){
                    this->outputStates.resize(0); 
                    for(int i = 0; i < this->inputCount; i++){

                        if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(this->vm_stack[i])){
                            auto state_hd = std::get<std::shared_ptr<HeapDescriptor>>(this->vm_stack[i]); 
                            this->outputStates.push_back(state_hd); 
                            
                            this->running = false; 
                        }
                    }
                }
                else{
                    this->vm_stack.resize(new_sz);
                }                
                break; 
            }
            else{
                throw std::runtime_error("What! Uneven scope! Language people do ur job"); 
            }
            break; 
        }    

        case(OPCODE::NOT) : {
            Prim latest = this->vm_stack.back(); 
            this->vm_stack.pop_back(); 

            if(std::holds_alternative<bool>(latest)){
                bool cond = std::get<bool>(latest); 
                this->vm_stack.push_back(!cond); 
            }
            else{
                throw std::runtime_error("Cannot NOT a non-boolean primative!"); 
            }
            break; 
        }
        default: 
            throw std::runtime_error("Unsupported OPCODE with OPKIND STACK!"); 
    }

}

void VM::actionHeap(Instruction &instr){
    switch(instr.code){

        case(OPCODE::MKHEAP) : {
            // Arg 2 is an integer code corresponding to the type of heap object to make
            /*
                0: Vector
                1: Map
            */

            std::string cType; 
            if(std::holds_alternative<std::string>(instr.arg1)){
                cType = std::get<std::string>(instr.arg1); 
            }

            if(instr.arg2 == 0){
                std::shared_ptr<VectorDescriptor> newHeapObj = std::make_shared<VectorDescriptor>(cType); 
                this->vm_stack.push_back(newHeapObj); 
            }
            else if(instr.arg2 == 1){
                std::shared_ptr<MapDescriptor> newHeapObj = std::make_shared<MapDescriptor>(cType); 
                this->vm_stack.push_back(newHeapObj); 
            }
            else{
                throw std::invalid_argument("Failed to intialize Heap object of ID: " + stringify(instr.arg2)); 
            }

            break; 
        }

        case(OPCODE::APPEND) : {
            // Object to insert
            Prim insertObj = this->vm_stack.back(); 
            this->vm_stack.pop_back(); 

            // Object to insert into; 
            Prim vectorDesc = this->vm_stack.back(); 

            std::shared_ptr<HeapDescriptor> vecHeap; 

            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(vectorDesc)){
                vecHeap = std::get<std::shared_ptr<HeapDescriptor>>(vectorDesc); 
            }
            else{
                throw std::invalid_argument("Invalid instr APPEND for NON-HEAP object"); 
            }

            
            if(std::shared_ptr<VectorDescriptor> vectorObj = std::dynamic_pointer_cast<VectorDescriptor>(vecHeap)){

                if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(insertObj)){
                    vectorObj->append(std::get<std::shared_ptr<HeapDescriptor>>(insertObj)); 
                }
                else{
                    auto primObj = std::make_shared<PrimDescriptor>(insertObj); 
                    vectorObj->append(primObj); 
                }
            }
            else{
                throw std::invalid_argument("Invalid instr APPEND for heap-object (must be vector)");
            }
            

            break; 
        }


        case(OPCODE::EMPLACE) : {
        
             // Object to insert 
            Prim insertObj = this->vm_stack.back();
            this->vm_stack.pop_back();  

            // Key to insert 
            Prim keyObj = this->vm_stack.back();
            this->vm_stack.pop_back();  

            // Dynamically cast to the MapDescriptor at runtime; 
            Prim mapObj = this->vm_stack.back(); 

            std::shared_ptr<HeapDescriptor> newHeap; 

            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(mapObj)){
                newHeap = std::get<std::shared_ptr<HeapDescriptor>>(mapObj); 
            }
            else{
                throw std::invalid_argument("Invalid instr EMPLACE for NON_HEAP object"); 
            }


            if(auto mapDesc = std::dynamic_pointer_cast<MapDescriptor>(newHeap)){
                 
                if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(insertObj)){
                    mapDesc->emplace(keyObj, std::get<std::shared_ptr<HeapDescriptor>>(insertObj)); 
                }
                else{
                    auto primObj = std::make_shared<PrimDescriptor>(insertObj); 
                    mapDesc->emplace(keyObj, primObj); 
                }
            }
            else{
                throw std::invalid_argument("Invalid instr EMPLACE for heap-object (must be map)");
            }

            break; 
        }

        case(OPCODE::ACC) : {
            auto accessor = resolvePrim(this->vm_stack.back()); 
            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(accessor)){
                throw std::invalid_argument("Cannot yet use non-primative heap descriptors to access heap objects"); 
            }

            this->vm_stack.pop_back(); 

            auto controlObj = this->vm_stack.back(); 
            this->vm_stack.pop_back(); 

            std::shared_ptr<HeapDescriptor> jamar; 

            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(controlObj)){
                auto heap_obj = std::get<std::shared_ptr<HeapDescriptor>>(controlObj); 
                std::shared_ptr<HeapDescriptor> bob = heap_obj->access(accessor); 

                // just push the plain descriptor over
                this->vm_stack.push_back(bob); 
                
            }

            break; 
        }
        case(OPCODE::SIZE) : {
            Prim top = this->vm_stack.back() ;

            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(top)){
                auto von = std::get<std::shared_ptr<HeapDescriptor>>(top); 
                this->vm_stack.pop_back(); 


                this->vm_stack.push_back(von->getSize()); 
    
            }
            else{
                throw std::invalid_argument("Cannot get size of a non-heap descriptor"); 
            }
            break; 

        }  
        default : {
            throw std::runtime_error("Unsupported OPCODE with OPKIND HEAP!"); 
        }

    }
}

void VM::actionControl(Instruction &instr){
    switch(instr.code){

        case(OPCODE::JMP) : {
            this->ip += instr.arg2;
            break; 
        }

        case(OPCODE::B_COND) : {
            // B_COND expects the top of the stack to hold a bool
            // Will branch to ip + arg2 if condition is false (arg2 can be negative); 
            Prim condition = this->vm_stack.back();
            this->vm_stack.pop_back();  

            if(std::holds_alternative<bool>(condition)){
                bool cond = std::get<bool>(condition); 
                if(!cond){
                    this->ip += instr.arg2; 
                }
            }
            else{
                throw std::runtime_error("Attempted to use instr B_COND when top of stack is not boolean"); 
            }
            break; 
        }


        // reset oblock loop at the end of the oblock
        case(OPCODE::OBLOCK_LOOP) : {
            this->ip = this->bytecode.begin() + this->code_offset; 
            break; 
        }

        case(OPCODE::END) : {
            this->running = false; 
            break; 
        }

        default : {
            throw std::runtime_error("Unsupported OPCODE with OPKIND CONTROL!"); 
        }

    }


}

void VM::actionTrap(Instruction &instr){
    switch(instr.code){
        // Prints to terminal
        case(OPCODE::PRINT) : {
            Prim obj = this->vm_stack.back() ;
            this->vm_stack.pop_back(); 
            std::cout<<stringify(resolvePrim(obj)); 
            break; 
        }
        // prints a newline to terminal
        case(OPCODE::PRINTLN) : {
            Prim obj = this->vm_stack.back(); 
            this->vm_stack.pop_back();
            std::cout<<stringify(resolvePrim(obj))<<std::endl; 
            break; 
        }
        default : {
            throw std::runtime_error("Unsupported OPCODE with OPKIND TRAP!"); 
        }

    }

}

void VM::action(Instruction &code){
    switch(code.kind){
        case OPKIND::STACK : {
            actionStack(code); 
            break; 
        }
        case OPKIND::BINARY: {
            actionBinary(code); 
            break; 
        }
        case OPKIND::HEAP :{
            actionHeap(code); 
            break; 
        }
        case OPKIND::CONTROL : {
            actionControl(code); 
            break; 
        }
        case OPKIND::TRAP : {   
            actionTrap(code); 
            break; 
        }
        case OPKIND::PREPROCESSING : {
            // Do nothing for preprocessing instructions (keep them to maintain offsets)
            break; 
        }        
        default :{
            throw std::runtime_error("Encountered unknown instruction type!"); 
        }
    }
} 



// This will be run in a continuous cycle

void VM::transform(std::vector<std::shared_ptr<HeapDescriptor>> &input, std::vector<std::shared_ptr<HeapDescriptor>> &output){
    // Transfer state into the first n elements of the vm_stack
    this->inputCount = input.size(); 

    this->vm_stack.clear(); 

    this->ip = this->bytecode.begin() + this->code_offset; 

    for(int i = 0 ; i < this->inputCount; i++){
        this->vm_stack.push_back(input[i]); 
    }

    this->running = true; 

    // Perform the transformations on the first n elements of the object: 
    while(this->running){
        action(*this->ip); 
        ip++; 
    }    

    // Load the elements into the shared_ptr
    if(this->outputStates.size() != this->inputCount){
        std::cout<<"\n\n"<<std::endl; 
        std::cout<<"ALERT!!"<<std::endl; 
        std::cout<<"Input size: "<<this->inputCount<<std::endl; 
        std::cout<<"output size: "<<this->outputStates.size()<<std::endl; 
        
        //throw std::invalid_argument("Bruh wtf"); 
    }

    
    for(auto& obj : this->outputStates){
            output.push_back(obj); 
        }
   
}

