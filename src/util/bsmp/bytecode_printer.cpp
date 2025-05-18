#include "bytecode_printer.hpp"
#include "include/Common.hpp"
#include "libtypes/bls_types.hpp"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <boost/json.hpp>
#include <memory>
#include <stdexcept>

using namespace boost::json;

void tag_invoke(const value_from_tag&, value& jv, DeviceDescriptor const & desc) {
    auto& obj = jv.emplace_object();
    obj.emplace("device_name", value_from(desc.device_name));
    obj.emplace("devtype", value_from(static_cast<uint32_t>(desc.devtype)));
    obj.emplace("controller", value_from(desc.controller));
    obj.emplace("port_maps", value_from(desc.port_maps));
    obj.emplace("isInterrupt", value_from(desc.isInterrupt));
    obj.emplace("isVtype", value_from(desc.isVtype));
    obj.emplace("isConst", value_from(desc.isConst));
    obj.emplace("polling_period", value_from(desc.polling_period));
}

void tag_invoke(const value_from_tag&, value& jv, OBlockDesc const & desc) {
    auto& obj = jv.emplace_object();
    obj.emplace("name", value_from(desc.name));
    obj.emplace("binded_devices", value_from(desc.binded_devices));
    obj.emplace("bytecode_offset", value_from(desc.bytecode_offset));
    obj.emplace("dropRead", value_from(desc.dropRead));
    obj.emplace("dropWrite", value_from(desc.dropWrite));
    obj.emplace("synchronize_states", value_from(desc.synchronize_states));
}

void tag_invoke(const value_from_tag&, value& jv, std::shared_ptr<HeapDescriptor> const & hd) {
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