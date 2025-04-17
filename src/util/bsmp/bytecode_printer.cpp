#include "bytecode_printer.hpp"
#include "libtypes/bls_types.hpp"
#include <cstdint>
#include <iostream>
#include <boost/json.hpp>
#include <memory>
#include <stdexcept>

using namespace boost::json;

void tag_invoke(const value_from_tag&, value& jv, std::shared_ptr<HeapDescriptor> const & hd)
{
    switch (hd->getType()) {
        case TYPE::list_t:
            jv = value_from(std::dynamic_pointer_cast<VectorDescriptor>(hd)->getVector());
        break;
        case TYPE::map_t:
            jv = value_from(std::dynamic_pointer_cast<MapDescriptor>(hd)->getMap());
        break;
        default:
            throw std::runtime_error("invalid heap descriptor");
        break;
    }
}

static void pretty_print( std::ostream& os, value const& jv, bool init = false)
{
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
                pretty_print(os, it->value());
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
                pretty_print( os, *it);
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

static std::ostream& operator<<(std::ostream& os, uint8_t& num) {
    os << static_cast<uint16_t>(num);
    return os;
}

void BytecodePrinter::setOutputStream(std::ostream& stream) {
    outputStream = &stream;
}

void BytecodePrinter::printHeader() {
    *outputStream << "LITERALS_BEGIN" << std::endl;
}

void BytecodePrinter::printLiteralPool() {
    auto json = value_from(literalPool);
    pretty_print(*outputStream, json, true);
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
    *outputStream << #code; \
    printArgs(args);\
    *outputStream << std::endl; \
    }
#include "libbytecode/include/OPCODES.LIST"
#undef OPCODE_BEGIN
#undef ARGUMENT
#undef OPCODE_END