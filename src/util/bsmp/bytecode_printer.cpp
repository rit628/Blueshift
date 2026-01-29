

#include "bytecode_printer.hpp"
#include "Serialization.hpp"
#include "bls_types.hpp"
#include "traps.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <boost/json.hpp>
#include <memory>
#include <stdexcept>

using namespace boost::json;

static void prettyPrintLiteralPool(std::ostream& os, value const& jv, bool init = false) {
    switch(jv.kind())
    {
    case kind::object:
    {
        os << "{";
        auto const& obj = jv.get_object();
        if(! obj.empty())
        {
            auto it = obj.begin();
            for(;;)
            {
                os << serialize(it->key()) << " : ";
                prettyPrintLiteralPool(os, it->value());
                if(++it == obj.end())
                    break;
                os << ", ";
            }
        }
        os << "}";
        break;
    }

    case kind::array:
    {
        os << "[";
        if (init) {
            os << "\n";
        }
        auto const& arr = jv.get_array();
        if(! arr.empty())
        {
            auto it = arr.begin();
            for(;;)
            {
                prettyPrintLiteralPool( os, *it);
                if(++it == arr.end())
                    break;
                if (init) {
                    os << ",\n";                    
                }
                else {
                    os << ", ";
                }
            }
        }
        if (init) {
            os << "\n";
        }
        os << "]";
        break;
    }

    case kind::string:
    {
        os << serialize(jv.get_string());
        break;
    }

    case kind::uint64:
    case kind::int64:
        os << jv;
        break;
    case kind::double_:
    {
        double temp = value_to<double>(jv);
        os << temp;
        break;
    }
    case kind::bool_:
        if(jv.get_bool())
            os << "true";
        else
            os << "false";
        break;

    case kind::null:
        os << "null";
        break;
    }

    if(init)
        os << "\n";
}

static void prettyPrintHeader(std::ostream& os, value const& jv, std::string* indent = nullptr) {
    std::string indent_;
    if(! indent)
        indent = &indent_;
    switch(jv.kind())
    {
    case kind::object:
    {
        os << "{\n";
        indent->append(4, ' ');
        auto const& obj = jv.get_object();
        if(! obj.empty())
        {
            auto it = obj.begin();
            for(;;)
            {
                os << *indent << serialize(it->key()) << " : ";
                prettyPrintHeader(os, it->value(), indent);
                if(++it == obj.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "}";
        break;
    }

    case kind::array:
    {
        os << "[\n";
        indent->append(4, ' ');
        auto const& arr = jv.get_array();
        if(! arr.empty())
        {
            auto it = arr.begin();
            for(;;)
            {
                os << *indent;
                prettyPrintHeader( os, *it, indent);
                if(++it == arr.end())
                    break;
                os << ",\n";
            }
        }
        os << "\n";
        indent->resize(indent->size() - 4);
        os << *indent << "]";
        break;
    }

    case kind::string:
    {
        os << serialize(jv.get_string());
        break;
    }

    case kind::uint64:
    case kind::int64:
    case kind::double_:
        os << jv;
        break;

    case kind::bool_:
        if(jv.get_bool())
            os << "true";
        else
            os << "false";
        break;

    case kind::null:
        os << "null";
        break;
    }

    if(indent->empty())
        os << "\n";
}

static std::ostream& operator<<(std::ostream& os, uint8_t& num) {
    os << static_cast<uint16_t>(num);
    return os;
}

void BytecodePrinter::setOutputStream(std::ostream& stream) {
    outputStream = &stream;
}

void BytecodePrinter::printAll() {
    *outputStream << "METADATA_BEGIN" << std::endl;
    printMetadata();
    *outputStream << "HEADER_BEGIN" << std::endl;
    printHeader();
    *outputStream << "LITERALS_BEGIN" << std::endl;
    printLiteralPool();
    *outputStream << "BYTECODE_BEGIN" << std::endl;
    dispatch();
}

void BytecodePrinter::printMetadata() {
    std::unordered_map<std::string, std::pair<uint16_t, std::vector<std::string>&>> functionSymbols;
    for (auto&& [address, metadata] : functionMetadata) {
        functionSymbols.emplace(metadata.first, std::make_pair(address, std::ref(metadata.second)));
    }
    auto json = value_from(functionSymbols);
    prettyPrintHeader(*outputStream, json);
}

void BytecodePrinter::printHeader() {
    auto json = value_from(taskDescs);
    prettyPrintHeader(*outputStream, json);
}

void BytecodePrinter::printLiteralPool() {
    auto json = value_from(literalPool);
    prettyPrintLiteralPool(*outputStream, json, true);
}

template<typename... Args>
void BytecodePrinter::printArgs(Args... args) {
    ((*outputStream << " " << args), ...);
}

void BytecodePrinter::printCALL(uint16_t address, uint8_t argc) {
    printArgs(functionMetadata.at(address).first, argc);
}

void BytecodePrinter::printEMIT(uint8_t signal) {
    using enum SIGNAL;
    switch (static_cast<SIGNAL>(signal)) {
        case START:
            printArgs("START");
        break;
        case STOP:
            printArgs("STOP");
        break;
        default:
            printArgs("INVALID");
        break;
    }
}

void BytecodePrinter::printPUSH(uint8_t index) {
    *outputStream << " ";
    prettyPrintLiteralPool(*outputStream, value_from(literalPool.at(index)));
}

void BytecodePrinter::printMKTYPE(uint8_t index, uint8_t type) {
    printArgs(currentFunctionSymbols->at(index), getTypeName(static_cast<TYPE>(type)));
}

void BytecodePrinter::printSTORE(uint8_t index) {
    printArgs(currentFunctionSymbols->at(index));
}

void BytecodePrinter::printLOAD(uint8_t index) {
    printArgs(currentFunctionSymbols->at(index));
}

void BytecodePrinter::printMTRAP(uint16_t callnum) {
    using namespace BlsTrap;
    printArgs(getMTrapName(static_cast<BlsTrap::MCALLNUM>(callnum)));
}

void BytecodePrinter::printTRAP(uint16_t callnum, uint8_t argc) {
    using namespace BlsTrap;
    printArgs(getTrapName(static_cast<BlsTrap::CALLNUM>(callnum)), argc);
}

#define OPCODE_BEGIN(code) \
void BytecodePrinter::code(
#define ARGUMENT(arg, type) \
    type arg,
#define OPCODE_END(code, args...) \
    int) { \
    if (functionMetadata.contains(instruction - 1)) { \
        auto& metadata = functionMetadata.at(instruction - 1); \
        auto& functionLabel = metadata.first; \
        currentFunctionSymbols = &metadata.second; \
        *outputStream << functionLabel << ":\n"; \
    } \
    *outputStream << "[" << instruction - 1 << "] " << #code; \
    if constexpr (OPCODE::code == OPCODE::CALL) { \
        printCALL(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::EMIT) { \
        printEMIT(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::PUSH) { \
        printPUSH(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::MKTYPE) { \
        printMKTYPE(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::STORE) { \
        printSTORE(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::LOAD) { \
        printLOAD(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::MTRAP) { \
        printMTRAP(args); \
    } \
    else if constexpr (OPCODE::code == OPCODE::TRAP) { \
        printTRAP(args); \
    } \
    else { \
        printArgs(args);\
    } \
    *outputStream << std::endl; \
    }
#include "include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END
