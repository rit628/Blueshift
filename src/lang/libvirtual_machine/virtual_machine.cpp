#include "virtual_machine.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "libtypes/bls_types.hpp"
#include "libtrap/include/traps.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <variant>
#include <vector>
#include <boost/range/iterator_range_core.hpp>

using namespace BlsLang;

void VirtualMachine::transform(size_t oblockOffset, std::vector<BlsType>& deviceStates) {
    instruction = oblockOffset;
    signal = SIGNAL::START;
    cs.pushFrame(instruction, deviceStates);
    dispatch();
    cs.popFrame();
}

void VirtualMachine::CALL(uint16_t address, uint8_t argc, int) {
    std::vector<BlsType> args;
    for (uint8_t i = 0; i < argc; i++) {
        args.push_back(cs.popOperand());
    }
    cs.pushFrame(instruction, args);
    instruction = address;
}

void VirtualMachine::EMIT(uint8_t signal, int) {
    this->signal = static_cast<SIGNAL>(signal);
}

void VirtualMachine::PUSH(uint8_t index, int) {
    auto value = literalPool[index];
    cs.pushOperand(value);
}

void VirtualMachine::MKTYPE(uint8_t index, uint8_t type, int) {
    TYPE objType = static_cast<TYPE>(type);
    BlsType value;
    switch (objType) {
        case TYPE::bool_t:
            value = bool(false);
        break;
        case TYPE::int_t:
            value = int64_t(0);
        break;
        case TYPE::float_t:
            value = double(0.0);
        break;
        case TYPE::string_t:
            value = std::string("");
        break;
        case TYPE::list_t:
            value = std::make_shared<VectorDescriptor>(TYPE::ANY);
        break;
        case TYPE::map_t:
            value = std::make_shared<MapDescriptor>(TYPE::ANY);
        break;
        default:
            value = std::monostate();
        break;
    }
    cs.addLocal(index, value);
}

void VirtualMachine::STORE(uint8_t index, int) {
    auto value = cs.popOperand();
    cs.getLocal(index) = value;
}

void VirtualMachine::LOAD(uint8_t index, int) {
    auto variable = cs.getLocal(index);
    cs.pushOperand(variable);
}

void VirtualMachine::ASTORE(int) {
    auto value = cs.popOperand();
    auto index = cs.popOperand();
    auto object = cs.popOperand();
    std::get<std::shared_ptr<HeapDescriptor>>(object)->access(index) = value;
}

void VirtualMachine::ALOAD(int) {
    auto index = cs.popOperand();
    auto object = cs.popOperand();
    auto value = std::get<std::shared_ptr<HeapDescriptor>>(object)->access(index);
    cs.pushOperand(value);
}

void VirtualMachine::NOT(int) {
    auto op = cs.popOperand();
    cs.pushOperand(-op);
}

void VirtualMachine::NEG(int) {
    auto op = cs.popOperand();
    cs.pushOperand(!op);
}

void VirtualMachine::OR(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs || rhs);
}

void VirtualMachine::AND(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs && rhs);
}

void VirtualMachine::LT(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs < rhs);
}

void VirtualMachine::LE(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs <= rhs);
}

void VirtualMachine::GT(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs > rhs);
}

void VirtualMachine::GE(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs >= rhs);
}

void VirtualMachine::EQ(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs == rhs);
}

void VirtualMachine::NE(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs != rhs);
}

void VirtualMachine::ADD(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs + rhs);
}

void VirtualMachine::SUB(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs - rhs);
}

void VirtualMachine::MUL(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs * rhs);
}

void VirtualMachine::DIV(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs / rhs);
}

void VirtualMachine::MOD(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs % rhs);
}

void VirtualMachine::EXP(int) {
    auto rhs = cs.popOperand();
    auto lhs = cs.popOperand();
    cs.pushOperand(lhs ^ rhs);
}

void VirtualMachine::JMP(uint16_t address, int) {
    instruction = address;
}

void VirtualMachine::BRANCH(uint16_t address, int) {
    auto condition = cs.popOperand();
    if (!condition) {
        instruction = address;
    }
}

void VirtualMachine::RETURN(int) {
    auto result = cs.popOperand();
    instruction = cs.popFrame();
    if (!std::holds_alternative<std::monostate>(result)) {
        cs.pushOperand(result);
    }
}

void VirtualMachine::EMPLACE(int) {
    auto value = cs.popOperand();
    auto key = cs.popOperand();
    auto object = cs.popOperand();
    auto map = std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(object));
    map->emplace(key, value);
}

void VirtualMachine::APPEND(int) {
    auto value = cs.popOperand();
    auto object = cs.popOperand();
    auto list = std::dynamic_pointer_cast<VectorDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(object));
    list->append(value);
}

void VirtualMachine::SIZE(int) {
    auto object = cs.popOperand();
    auto size = std::get<std::shared_ptr<HeapDescriptor>>(object)->getSize();
    cs.pushOperand(size);
}

void VirtualMachine::TRAP(uint16_t callnum, uint8_t argc, int) {
    std::vector<BlsType> args;
    for (uint8_t i = 0; i < argc; i++) {
        args.push_back(cs.popOperand());
    }

    BlsTrap::CALLNUM trapnum = static_cast<BlsTrap::CALLNUM>(callnum);
    switch (trapnum) {
        #define TRAP_BEGIN(name, ...) \
        case BlsTrap::CALLNUM::name: { \
            BlsTrap::exec__##name(args);
            #define VARIADIC(...)
            #define ARGUMENT(argName, argType...)
            #define TRAP_END \
            break; \
        }
        #include "libtrap/include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END
        default:
        break;
    }
}