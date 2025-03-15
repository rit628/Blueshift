#include "bytecode_printer.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <cstdint>

void BytecodePrinter::CALL(uint16_t address, int) {
    std::cout << "IN CALL address: " << address << std::endl;
}

void BytecodePrinter::PUSH(uint8_t index, int) {

}

void BytecodePrinter::STORE(uint8_t index, int) {

}

void BytecodePrinter::LOAD(uint8_t index, int) {

}

void BytecodePrinter::INC(uint8_t index, int) {

}

void BytecodePrinter::DEC(uint8_t index, int) {

}

void BytecodePrinter::NOT(int) {
    std::cout << "IN NOT" << std::endl;
}

void BytecodePrinter::NEG(int) {

}

void BytecodePrinter::OR(int) {

}

void BytecodePrinter::AND(int) {

}

void BytecodePrinter::LT(int) {
    std::cout << "IN LT" << std::endl;
}

void BytecodePrinter::LE(int) {

}

void BytecodePrinter::GT(int) {

}

void BytecodePrinter::GE(int) {

}

void BytecodePrinter::EQ(int) {

}

void BytecodePrinter::NE(int) {

}

void BytecodePrinter::ADD(int) {

}

void BytecodePrinter::SUB(int) {

}

void BytecodePrinter::MUL(int) {

}

void BytecodePrinter::DIV(int) {

}

void BytecodePrinter::MOD(int) {

}

void BytecodePrinter::EXP(int) {

}

void BytecodePrinter::JMP(uint16_t address, int) {

}

void BytecodePrinter::BRANCH(uint16_t address, int) {

}

void BytecodePrinter::RETURN(int) {

}

void BytecodePrinter::MKHEAP(uint8_t type, int) {

}

void BytecodePrinter::ACC(int) {

}

void BytecodePrinter::EMPLACE(int) {

}

void BytecodePrinter::APPEND(int) {

}

void BytecodePrinter::SIZE(int) {

}

void BytecodePrinter::PRINT(int) {

}

void BytecodePrinter::PRINTLN(int) {

}