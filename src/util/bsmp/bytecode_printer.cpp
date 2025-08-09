

#include "bytecode_printer.hpp"
#include "include/Common.hpp"
#include "libtype/bls_types.hpp"
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

void BytecodePrinter::printHeader() {
    std::for_each(oblockDescs.begin(), oblockDescs.end(), [this](const auto& desc){
       oblockLabels.emplace(desc.bytecode_offset, desc.name);
    });
    auto json = value_from(oblockDescs);
    prettyPrintHeader(*outputStream, json);
    *outputStream << "LITERALS_BEGIN" << std::endl;
}

void BytecodePrinter::printLiteralPool() {
    auto json = value_from(literalPool);
    prettyPrintLiteralPool(*outputStream, json, true);
    *outputStream << "BYTECODE_BEGIN" << std::endl;
}

template<typename... Args>
void BytecodePrinter::printArgs(Args... args) {
    ((*outputStream << " " << args), ...);
}

#define OPCODE_BEGIN(code) \
void BytecodePrinter::code(
#define ARGUMENT(arg, type) \
    type arg,
#define OPCODE_END(code, args...) \
    int) { \
    if (oblockLabels.contains(instruction - 1)) { \
        *outputStream << '_' << oblockLabels.at(instruction - 1) << ":\n"; \
    } \
    *outputStream << #code; \
    printArgs(args);\
    *outputStream << std::endl; \
    }
#include "libbytecode/include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END