#include "optimizer.hpp"
#include "bytecode_processor.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include "traps.hpp"

using namespace BlsLang; 


MetadataFrame::MetadataFrame(std::span<PRMetadata> &args){
    this->locals.resize(args.size()); 
    // transfer args
    this->locals.assign(args.begin(), args.end());
}

void MetadataFrame::addLocal(PRMetadata &pr, int index){
    if(index >= this->locals.size()){
        this->locals.push_back(pr); 
    }
    else{
        this->locals.at(index) = pr; 
    }
}

void MetadataFrame::pushOperand(PRMetadata& pr){
    this->pr_stk.push(pr); 
}

PRMetadata MetadataFrame::popOperand(){
    auto temp = this->pr_stk.top(); 
    this->pr_stk.pop(); 
    return temp; 
}

PRMetadata& MetadataFrame::getLocal(int index){
    return this->locals.at(index); 
}

void MetadataFrame::setLocal(int index, PRMetadata& metadata){
    if(index < this->locals.size()){
        this->locals[index] = metadata; 
    }
    else{
        throw std::invalid_argument("Bad indexing for local access"); 
    }
}

void MetadataStack::pushFrame(int ret, std::span<PRMetadata> prArgs){
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


void Optimizer::optimize(){
    // Populate the initial task descriptor list: 
    for(auto& desc : this->getTaskDescriptors()){
        this->metadataStack.pushFrame(instruction, {});
        auto& frame = this->metadataStack.getCurrentFrame(); 
        instruction = desc.bytecode_offset; 
        int i = 0; 
        for(auto& devDesc : desc.binded_devices){
            PRMetadata pr{.deviceDeps = {devDesc.device_name}, 
                        .controllerDeps = {devDesc.controller},
                        .og = ORIGIN::LOCALS, .index = i}; 
            frame.addLocal(pr, i); 
            i++; 
        }   
        dispatch(); 
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
    PRMetadata pm{.og = ORIGIN::LITERAL, .index = index};
    this->metadataStack.getCurrentFrame().pushOperand(pm);  

}

void Optimizer::MKTYPE(uint8_t index, uint8_t type, int){
    // Adds the type information to the local: 
    PRMetadata pm {.type = static_cast<TYPE>(type), .og = ORIGIN::LOCALS, .index = index}; 
    this->metadataStack.getCurrentFrame().addLocal(pm, index); 
}

void Optimizer::STORE(uint8_t index, int) {
    // Stores the elements back in the local: 
    auto& frame = this->metadataStack.getCurrentFrame(); 
    auto newValue = frame.popOperand(); 
    frame.setLocal(index, newValue); 
}

void Optimizer::LOAD(uint8_t index, int) {
    // Loads in metadata information from the locals: 
    auto& frame = this->metadataStack.getCurrentFrame(); 
    auto obj = frame.getLocal(index); 
    frame.pushOperand(obj); 
}

void Optimizer::ASTORE(int) {




}

void Optimizer::ALOAD(int) {
    

}


void Optimizer::NOT(int) { 
    // Does not change the symbol metadata
    return; 
}

void Optimizer::NEG(int) {

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

}

void Optimizer::SUB(int) {

}

void Optimizer::MUL(int) {

}

void Optimizer::DIV(int) {

}

void Optimizer::MOD(int) {

}

void Optimizer::EXP(int) {

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



