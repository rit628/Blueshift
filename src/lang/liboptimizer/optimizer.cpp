#include "optimizer.hpp"
#include "bytecode_processor.hpp"
#include <iostream>
#include "traps.hpp"

using namespace BlsLang; 


void Optimizer::optimize(){
    dispatch(); 
}

void Optimizer::CALL(uint16_t address, uint8_t argc, int){

}

void Optimizer::EMIT(uint8_t signal, int){
    if(static_cast<SIGNAL>(signal) == SIGNAL::STOP){
        std::cout<<"Task Concluded!"<<std::endl; 
    }
}

void Optimizer::PUSH(uint8_t index, int) {
    std::cout<<"Pushing Device State!"<<std::endl; 
}

void Optimizer::MKTYPE(uint8_t index, uint8_t type, int){
    std::cout<<"Making new device type!"<<std::endl; 

}

void Optimizer::STORE(uint8_t index, int) {

}

void Optimizer::LOAD(uint8_t index, int) {
    std::cout<<"Loading new device state"<<std::endl; 

}

void Optimizer::ASTORE(int) {

}

void Optimizer::ALOAD(int) {

}

void Optimizer::NOT(int) {

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



