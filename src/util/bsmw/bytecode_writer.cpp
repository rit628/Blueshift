#include "bytecode_writer.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtypes/bls_types.hpp"
#include "libbytecode/include/opcodes.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>
#include <boost/json.hpp>
#include <stdexcept>
#include <string>
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

void BytecodeWriter::loadMnemonicBytecode(const std::string& filename) {
    std::fstream input;
    input.open(filename);
    if (!input.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    mnemonicBytecode << input.rdbuf();
}

void BytecodeWriter::writeHeader(std::ostream& stream) {
    std::string buf, poolJSON;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != "LITERALS_BEGIN")) {
        poolJSON += buf;
    }
}

void BytecodeWriter::writeLiteralPool(std::ostream& stream) {
    std::string buf, poolJSON;
    while ((std::getline(mnemonicBytecode, buf)) && (buf != "BYTECODE_BEGIN")) {
        poolJSON += buf;
    }
    array pool = parse(poolJSON).get_array();
    std::function<BlsType(value&)> convertToBlsType = [&](value& literal) -> BlsType {
        if (literal.is_array()) {
            auto list = std::make_shared<VectorDescriptor>(TYPE::ANY);
            for (auto&& element : literal.get_array()) {
                auto converted = convertToBlsType(element);
                list->append(converted);
            }
            return list;
        }
        else if (literal.is_object()) {
            auto map = std::make_shared<MapDescriptor>(TYPE::ANY);
            for (auto&& [key, value] : literal.get_object()) {
                auto convertedKey = BlsType(key);
                auto convertedValue = convertToBlsType(value);
                map->emplace(convertedKey, convertedValue);
            }
            return map;
        }
        else if (literal.is_bool()) {
            return value_to<bool>(literal);
        }
        else if (literal.is_int64()) {
            return value_to<int64_t>(literal);
        }
        else if (literal.is_double()) {
            return value_to<double>(literal);
        }
        else if (literal.is_string()) {
            return value_to<std::string>(literal);
        }
        else {
            return std::monostate();
        }
    };

    uint16_t poolSize = pool.size();
    stream.write(reinterpret_cast<const char *>(&poolSize), sizeof(poolSize));
    
    for (auto&& literal : pool) {
        auto converted = convertToBlsType(literal);
        std::visit(overloads {
            [&](bool& value) {
                auto type = TYPE::bool_t;
                stream.write(reinterpret_cast<const char *>(&type), sizeof(type));
                stream.write(reinterpret_cast<const char *>(&value), sizeof(value));
            },
            [&](int64_t& value) {
                auto type = TYPE::int_t;
                stream.write(reinterpret_cast<const char *>(&type), sizeof(type));
                stream.write(reinterpret_cast<const char *>(&value), sizeof(value));
            },
            [&](double& value) {
                auto type = TYPE::float_t;
                stream.write(reinterpret_cast<const char *>(&type), sizeof(type));
                stream.write(reinterpret_cast<const char *>(&value), sizeof(value));
            },
            [&](std::string& value) {
                auto type = TYPE::string_t;
                auto size = value.size();
                stream.write(reinterpret_cast<const char *>(&type), sizeof(type));
                stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
                stream.write(value.c_str(), size);
            },
            [&](std::shared_ptr<HeapDescriptor>& value) {
                auto type = value->getType();
                // Nested data structures and lists dont work at the moment
                if (type == TYPE::list_t) {
                    auto temp = std::make_shared<MapDescriptor>(TYPE::ANY);
                    BlsType key = "__SERIALIZED_LIST__";
                    BlsType val = value;
                    temp->emplace(key, val);
                    value = temp;
                }
                DynamicMessage dmsg;
                dmsg.makeFromRoot(value);
                auto serialized = dmsg.Serialize();
                auto size = serialized.size();
                stream.write(reinterpret_cast<const char *>(&type), sizeof(type));
                stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
                stream.write(serialized.data(), size);
            },
            [](auto& value) {
                throw std::runtime_error("invalid literal");
            }
        }, converted);
    }
}

void BytecodeWriter::writeBody(std::ostream& stream) {
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
        else {
            throw std::runtime_error("INVALID OPCODE: " + buf);
        }
    }
}

void BytecodeWriter::convertToBinary(std::ostream& stream) {
    writeHeader(stream);
    writeLiteralPool(stream);
    writeBody(stream);
}