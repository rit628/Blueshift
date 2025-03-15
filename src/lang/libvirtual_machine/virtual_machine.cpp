#include "virtual_machine.hpp"
#include <cstdint>

using namespace BlsLang;

void VirtualMachine::CALL(uint16_t address, int) {

}

void VirtualMachine::PUSH(uint8_t index, int) {

}

void VirtualMachine::STORE(uint8_t index, int) {

}

void VirtualMachine::LOAD(uint8_t index, int) {

}

void VirtualMachine::INC(uint8_t index, int) {

}

void VirtualMachine::DEC(uint8_t index, int) {

}

void VirtualMachine::NOT(int) {

}

void VirtualMachine::NEG(int) {
    auto op = cs.popOperand();
    cs.pushOperand(std::move(!op));
}

void VirtualMachine::OR(int) {

}

void VirtualMachine::AND(int) {

}

void VirtualMachine::LT(int) {

}

void VirtualMachine::LE(int) {

}

void VirtualMachine::GT(int) {

}

void VirtualMachine::GE(int) {

}

void VirtualMachine::EQ(int) {

}

void VirtualMachine::NE(int) {

}

void VirtualMachine::ADD(int) {

}

void VirtualMachine::SUB(int) {

}

void VirtualMachine::MUL(int) {

}

void VirtualMachine::DIV(int) {

}

void VirtualMachine::MOD(int) {

}

void VirtualMachine::EXP(int) {

}

void VirtualMachine::JMP(uint16_t address, int) {

}

void VirtualMachine::BRANCH(uint16_t address, int) {

}

void VirtualMachine::RETURN(int) {

}

void VirtualMachine::MKHEAP(uint8_t type, int) {

}

void VirtualMachine::ACC(int) {

}

void VirtualMachine::EMPLACE(int) {

}

void VirtualMachine::APPEND(int) {

}

void VirtualMachine::SIZE(int) {

}

void VirtualMachine::PRINT(int) {

}

void VirtualMachine::PRINTLN(int) {

}