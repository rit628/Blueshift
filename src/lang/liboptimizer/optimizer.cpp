#include "optimizer.hpp"
#include "bytecode_processor.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include "opcodes.hpp"
#include "traps.hpp"

using namespace BlsLang; 



MetadataFrame::MetadataFrame(std::span<TaskMetadata> &args){
    this->locals.resize(args.size()); 
    // transfer args
    this->locals.assign(args.begin(), args.end());
}

void MetadataFrame::addLocal(TaskMetadata &pr, int index){
    if(index >= this->locals.size()){
        this->locals.push_back(pr); 
    }
    else{
        this->locals.at(index) = pr; 
    }
}

void MetadataFrame::pushOperand(TaskMetadata& pr){
    this->task_stk.push(pr); 
}

TaskMetadata MetadataFrame::popOperand(){
    auto temp = this->task_stk.top(); 
    this->task_stk.pop(); 
    return temp; 
}

TaskMetadata& MetadataFrame::topOperand(){
    auto& temp = this->task_stk.top(); 
    return temp; 
}

TaskMetadata& MetadataFrame::getLocal(int index){
    return this->locals.at(index); 
}

void MetadataFrame::setLocal(int index, TaskMetadata& metadata){
    if(index < this->locals.size()){
        this->locals[index] = metadata; 
    }
    else{
        throw std::invalid_argument("Bad indexing for local access"); 
    }
}

void MetadataStack::pushFrame(int ret, std::span<TaskMetadata> prArgs){
    MetadataFrame mf{prArgs}; 
    mf.returnAddress = ret;
    this->metadataStack.push(mf);  
}

int MetadataStack::popFrame(){
    MetadataFrame mf = this->metadataStack.top(); 
    this->metadataStack.pop(); 
    return mf.returnAddress; 
}

MetadataFrame& MetadataStack::getCurrentFrame(){
    return this->metadataStack.top(); 
}

TaskMetadata Optimizer::combineDeps(std::vector<TaskMetadata>&& argVect){
    TaskMetadata pr; 

    for(auto &a : argVect){
        for(auto& dep : a.controllerDeps){
            if(!this->loadCtlDeps.contains(dep)){
                std::cout<<"Adding Dependency: "<<dep<<" at "<<instruction<<std::endl; 
                loadCtlDeps.insert(dep); 
            }
        }

        pr.controllerDeps.insert(a.controllerDeps.begin(), a.controllerDeps.end()); 

        for(auto &task : a.taskDeps){
            pr.taskDeps.push_back(task); 
        }

        if(!a.complete){
            pr.instructionDeps.insert(a.instructionDeps.begin(), a.instructionDeps.end());
        }
    }

    return pr; 
}

void Optimizer::divideTree(int index){
    divideTreeH(this->metadataStack.getCurrentFrame().getLocal(index)); 
}


void Optimizer::divideTreeH(TaskMetadata &tm){

    for(auto& dep : tm.controllerDeps){
        std::cout<<dep<<std::endl; 
    }
    std::cout<<"------------------------------"<<std::endl; 

    for(auto& dep : tm.taskDeps){
        divideTreeH(dep); 
    }
}


void Optimizer::optimize(){
    // Populate the initial task descriptor list: 
    for(auto& desc : this->getTaskDescriptors()){
        this->metadataStack.pushFrame(instruction, {});
        auto& frame = this->metadataStack.getCurrentFrame(); 
        instruction = desc.bytecode_offset; 
        int i = 0; 
        for(auto& devDesc : desc.binded_devices){
            TaskMetadata pr{
                        .controllerDeps = {devDesc.controller},
                        .og = ORIGIN::LOCALS, .index = i}; 
            frame.addLocal(pr, i); 
            i++; 
        }   
        dispatch(); 
        divideTree(2); 

        this->metadataStack.popFrame(); 
    }
    return; 
}


void Optimizer::CALL(uint16_t address, uint8_t argc, int){
    
}

void Optimizer::EMIT(uint8_t signal, int){
    // End of a task in bytecode
    if(static_cast<SIGNAL>(signal) == SIGNAL::STOP){
        std::cout<<"Encountered a signal stop"<<std::endl; 
    }
}

void Optimizer::PUSH(uint8_t index, int) {
    // Adds the metadata info from the literal pool to the stack
    TaskMetadata pm{
        .instructionDeps = {instruction}, 
        .og = ORIGIN::LITERAL, 
        .index = index, 
    };

    this->metadataStack.getCurrentFrame().pushOperand(pm);  

}

void Optimizer::MKTYPE(uint8_t index, uint8_t type, int){
    // Adds the type information to the local: 
    TaskMetadata pm {
        .instructionDeps = {instruction},
        .type = static_cast<TYPE>(type), 
        .og = ORIGIN::LOCALS, 
        .index = index, 
    }; 

    this->metadataStack.getCurrentFrame().addLocal(pm, index); 
}

void Optimizer::STORE(uint8_t index, int) {
    // Stores the elements back in the local: 
    auto& frame = this->metadataStack.getCurrentFrame(); 
    auto poppedOp = frame.popOperand(); 
    auto& currentLoc = frame.getLocal(index); 
    auto newValue = combineDeps({poppedOp, currentLoc}); 

    newValue.instructionDeps.insert(instruction); 
    newValue.complete = true;
    loadCtlDeps.clear(); 
    std::cout<<"----------------------------------------"<<std::endl; 
    frame.setLocal(index, newValue); 
}

void Optimizer::LOAD(uint8_t index, int) {
    // Loads in metadata information from the locals: 
    auto& frame = this->metadataStack.getCurrentFrame(); 
    auto obj = frame.getLocal(index); 
    TaskMetadata tm{.controllerDeps = obj.controllerDeps, 
                .instructionDeps = {instruction}, 
                .index = obj.index,
                .taskDeps = {obj}}; 


    frame.pushOperand(tm); 
}

void Optimizer::ASTORE(int) {
    auto& stk = this->metadataStack.getCurrentFrame();

    auto value =  stk.popOperand(); 
    auto index = stk.popOperand(); 
    auto object =  stk.popOperand(); 

    auto comb = combineDeps({value, index, object});

    comb.instructionDeps.insert(instruction); 
    comb.complete = true;

    loadCtlDeps.clear(); 

    stk.setLocal(object.index, comb); 
}

void Optimizer::ALOAD(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 

    auto attr = stk.popOperand(); 
    auto obj =  stk.popOperand(); 
    auto comb = combineDeps({obj, attr}); 
    comb.instructionDeps.insert(instruction); 
    TaskMetadata tm{.controllerDeps = comb.controllerDeps, 
                .instructionDeps = {comb.instructionDeps.begin(), comb.instructionDeps.end()}, 
                .index = comb.index,
                .taskDeps = comb.taskDeps}; 

    stk.pushOperand(comb); 
}


// Unary Operators
void Optimizer::NOT(int) { 
    auto& stk = this->metadataStack.getCurrentFrame(); 
    stk.topOperand().instructionDeps.insert(instruction); 
    return; 
}

void Optimizer::NEG(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    stk.topOperand().instructionDeps.insert(instruction); 
    return; 
}

void Optimizer::OR(int) {

}

void Optimizer::AND(int) {

}

void Optimizer::LT(int) {

}

void Optimizer::LE(int) {

}

void Optimizer::GT(int) {

}

void Optimizer::GE(int) {

}

void Optimizer::EQ(int) {

}

void Optimizer::NE(int) {

}

void Optimizer::ADD(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    auto op1 = stk.popOperand(); 
    auto op2 =  stk.popOperand(); 
    auto comb = combineDeps({op2, op1}); 
    comb.instructionDeps.insert(instruction);
    stk.pushOperand(comb); 
}

void Optimizer::SUB(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    auto op1 = stk.popOperand(); 
    auto op2 =  stk.popOperand(); 
    auto comb = combineDeps({op2, op1}); 
    comb.instructionDeps.insert(instruction);
    stk.pushOperand(comb); 
}

void Optimizer::MUL(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    auto op1 = stk.popOperand(); 
    auto op2 =  stk.popOperand(); 
    auto comb = combineDeps({op2, op1}); 
    comb.instructionDeps.insert(instruction);
    stk.pushOperand(comb); 

}

void Optimizer::DIV(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    auto op1 = stk.popOperand(); 
    auto op2 =  stk.popOperand(); 
    auto comb = combineDeps({op2, op1}); 
    comb.instructionDeps.insert(instruction);
    stk.pushOperand(comb);  

}

void Optimizer::MOD(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    auto op1 = stk.popOperand(); 
    auto op2 =  stk.popOperand(); 
    auto comb = combineDeps({op2, op1}); 
    comb.instructionDeps.insert(instruction);
    stk.pushOperand(comb); ; 

}

void Optimizer::EXP(int) {
    auto& stk = this->metadataStack.getCurrentFrame(); 
    auto op1 = stk.popOperand(); 
    auto op2 =  stk.popOperand(); 
    auto comb = combineDeps({op2, op1}); 
    comb.instructionDeps.insert(instruction);
    stk.pushOperand(comb);  

}

void Optimizer::JMP(uint16_t address, int) {

}

void Optimizer::BRANCH(uint16_t address, int) {

}

void Optimizer::RETURN(int) {

}

void Optimizer::TRAP(uint16_t callnum, uint8_t argc, int) {

}

void Optimizer::MTRAP(uint16_t callnum, int) {

}

void Optimizer::JMPSC_OR(uint16_t address, int) {

}

void Optimizer::JMPSC_AND(uint16_t address, int) {

}



