#include "bytecode_writer.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtype/bls_types.hpp"
#include "libbytecode/opcodes.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace boost::json;

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

static std::stringstream& operator>>(std::stringstream& ss, uint8_t& num) {
    uint16_t temp;
    ss >> temp;
    num = temp;
    return ss;
}

BlsType tag_invoke(const value_to_tag<BlsType>&, value const& jv) {
    if (jv.is_array()) {
        auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
        for (auto&& element : jv.get_array()) {
            auto converted = value_to<BlsType>(element);
            list->append(converted);
        }
        return list;
    }
    else if (jv.is_object()) {
        auto map = std::make_shared<MapDescriptor>(TYPE::ANY);
        for (auto&& [key, value] : jv.get_object()) {
            auto convertedKey = BlsType(key);
            auto convertedValue = value_to<BlsType>(value);
            map->add(convertedKey, convertedValue);
        }
        return map;
    }
    else if (jv.is_bool()) {
        return value_to<bool>(jv);
    }
    else if (jv.is_int64()) {
        return value_to<int64_t>(jv);
    }
    else if (jv.is_double()) {
        return value_to<double>(jv);
    }
    else if (jv.is_string()) {
        return value_to<std::string>(jv);
    }
    else {
        return std::monostate();
    }
}

DeviceDescriptor tag_invoke(const value_to_tag<DeviceDescriptor>&, value const& jv) {
    auto& obj = jv.as_object();
    DeviceDescriptor desc;
    desc.device_name = value_to<std::string>(obj.at("device_name"));
    desc.type = static_cast<TYPE>(value_to<uint32_t>(obj.at("type")));
    desc.controller = value_to<std::string>(obj.at("controller"));
    desc.port_maps = value_to<std::unordered_map<std::string, std::string>>(obj.at("port_maps"));
    desc.initialValue = value_to<BlsType>(obj.at("initialValue"));
    desc.isVtype = value_to<bool>(obj.at("isVtype"));
    desc.dropRead = value_to<bool>(obj.at("dropRead"));
    desc.dropWrite = value_to<bool>(obj.at("dropWrite"));
    desc.polling_period = value_to<int>(obj.at("polling_period"));
    desc.isConst = value_to<bool>(obj.at("isConst"));
    desc.isInterrupt = value_to<bool>(obj.at("isInterrupt"));
    desc.isCursor = value_to<bool>(obj.at("isCursor"));
    return desc;
}

TriggerData tag_invoke(const value_to_tag<TriggerData>&, value const& jv) {
    auto& obj = jv.as_object();
    TriggerData trigger;
    trigger.rule = value_to<std::vector<std::string>>(obj.at("rule"));
    trigger.id = value_to<std::optional<std::string>>(obj.at("id"));
    trigger.priority = value_to<uint8_t>(obj.at("priority"));
    return trigger;
}

OBlockDesc tag_invoke(const value_to_tag<OBlockDesc>&, value const& jv) {
    auto& obj = jv.as_object();
    OBlockDesc desc;
    desc.name = value_to<std::string>(obj.at("name"));
    desc.binded_devices = value_to<std::vector<DeviceDescriptor>>(obj.at("binded_devices"));
    desc.bytecode_offset = value_to<int>(obj.at("bytecode_offset"));
    desc.inDevices = value_to<std::vector<DeviceDescriptor>>(obj.at("inDevices"));
    desc.outDevices = value_to<std::vector<DeviceDescriptor>>(obj.at("outDevices"));
    desc.triggers = value_to<std::vector<TriggerData>>(obj.at("triggers"));
    return desc;
}

void BytecodeWriter::loadMnemonicBytecode(const std::string& filename) {
    std::fstream input;
    input.open(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    mnemonicBytecode << input.rdbuf();
}

void BytecodeWriter::writeHeader(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf, oblockDescJSON;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != "LITERALS_BEGIN")) {
        oblockDescJSON += buf;
    }
    array descArray = parse(oblockDescJSON).get_array();
    uint16_t descSize = descArray.size();
    stream.write(reinterpret_cast<const char *>(&descSize), sizeof(descSize));
    for (auto&& desc : descArray) {
        oa << value_to<OBlockDesc>(desc);
    }
}

void BytecodeWriter::writeLiteralPool(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf, poolJSON;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != "BYTECODE_BEGIN")) {
        poolJSON += buf;
    }
    array pool = parse(poolJSON).get_array();

    uint16_t poolSize = pool.size();
    stream.write(reinterpret_cast<const char *>(&poolSize), sizeof(poolSize));
    
    for (auto&& literal : pool) {
        oa << value_to<BlsType>(literal);
    }
}

void BytecodeWriter::writeBody(std::ostream& stream, boost::archive::binary_oarchive& oa) {
    std::string buf;
    while (mnemonicBytecode >> buf) {
        if (false) { } // hack to force short circuiting and invalid input checking
        #define OPCODE_BEGIN(code) \
        else if (buf == #code) { \
            OPCODE c = OPCODE::code; \
            stream.write(reinterpret_cast<const char *>(&c), sizeof(c));
        #define ARGUMENT(arg, type) \
            type arg; \
            mnemonicBytecode >> arg; \
            stream.write(reinterpret_cast<const char *>(&arg), sizeof(type));
        #define OPCODE_END(...) \
        }
        #include "libbytecode/include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END
        else if (buf.ends_with(':')) { // oblock/procedure labels
            continue;
        }
        else {
            throw std::runtime_error("INVALID OPCODE: " + buf);
        }
    }
}

void BytecodeWriter::convertToBinary(std::ostream& stream) {
    if (stream.bad()) {
        throw std::runtime_error("Bad output stream provided.");
    }
    boost::archive::binary_oarchive oa(stream, boost::archive::archive_flags::no_header);
    writeHeader(stream, oa);
    writeLiteralPool(stream, oa);
    writeBody(stream, oa);
}