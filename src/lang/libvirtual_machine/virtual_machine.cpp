#include "virtual_machine.hpp"
#include "libtypes/bls_types.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <variant>
#include <vector>
#include <boost/range/iterator_range_core.hpp>

using namespace BlsLang;

void VirtualMachine::CALL(uint16_t address, uint8_t argc, int) {
    std::vector<BlsType> args;
    for (uint8_t i = 0; i < argc; i++) {
        args.push_back(cs.popOperand());
    }
    cs.pushFrame(instruction, args);
    instruction = address;
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
    cs.setLocal(index, value);
}

void VirtualMachine::STORE(uint8_t index, int) {
    auto value = cs.popOperand();
    cs.getLocal(index) = value;
}

void VirtualMachine::LOAD(uint8_t index, int) {
    auto variable = cs.getLocal(index);
    cs.pushOperand(variable);
}

void VirtualMachine::INC(uint8_t index, int) {
    auto variable = cs.getLocal(index);
    variable = variable + 1;
    cs.pushOperand(variable);
}

void VirtualMachine::DEC(uint8_t index, int) {
    auto variable = cs.getLocal(index);
    variable = variable - 1;
    cs.pushOperand(variable);
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
    if (condition) {
        instruction = address;
    }
}

void VirtualMachine::RETURN(int) {
    auto result = cs.popOperand();
    cs.popFrame();
    if (!std::holds_alternative<std::monostate>(result)) {
        cs.pushOperand(result);
    }
}

void VirtualMachine::ACC(int) {
    auto index = cs.popOperand();
    auto object = cs.popOperand();
    auto value = std::get<std::shared_ptr<HeapDescriptor>>(object)->access(index);
    cs.pushOperand(value);
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

void VirtualMachine::PRINT(uint8_t argc, int) {
    std::vector<BlsType> args;
    for (uint8_t i = 0; i < argc; i++) {
        args.push_back(cs.popOperand());
    }
    if (args.size() > 0) {
        if (std::holds_alternative<int64_t>(args.at(0))) {
            std::cout << std::get<int64_t>(args.at(0)) << std::flush;
        }
        else if (std::holds_alternative<double>(args.at(0))) {
            std::cout << std::get<double>(args.at(0)) << std::flush;
        }
        else if (std::holds_alternative<bool>(args.at(0))) {
            std::cout << ((std::get<bool>(args.at(0))) ? "true" : "false") << std::flush;
        }
        else if (std::holds_alternative<std::string>(args.at(0))) {
            std::cout << std::get<std::string>(args.at(0)) << std::flush;
        }
        for (auto&& arg : boost::make_iterator_range(args.begin()+1, args.end())) {
            if (std::holds_alternative<int64_t>(arg)) {
                std::cout << " " << std::get<int64_t>(arg) << std::flush;
            }
            else if (std::holds_alternative<double>(arg)) {
                std::cout << " " << std::get<double>(arg) << std::flush;
            }
            else if (std::holds_alternative<bool>(arg)) {
                std::cout << " " << ((std::get<bool>(arg)) ? "true" : "false") << std::flush;
            }
            else if (std::holds_alternative<std::string>(arg)) {
                std::cout << " " << std::get<std::string>(arg) << std::flush;
            }
        }
        std::cout << std::endl;
    }
}

void VirtualMachine::PRINTLN(uint8_t argc, int) {
    std::vector<BlsType> args;
    for (uint8_t i = 0; i < argc; i++) {
        args.push_back(cs.popOperand());
    }
    for (auto&& arg : args) {
        if (std::holds_alternative<int64_t>(arg)) {
            std::cout << std::get<int64_t>(arg) << std::endl;
        }
        else if (std::holds_alternative<double>(arg)) {
            std::cout << std::get<double>(arg) << std::endl;
        }
        else if (std::holds_alternative<bool>(arg)) {
            std::cout << ((std::get<bool>(arg)) ? "true" : "false") << std::endl;
        }
        else if (std::holds_alternative<std::string>(arg)) {
            std::cout << std::get<std::string>(arg) << std::endl;
        }
    }
}