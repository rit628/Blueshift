#pragma once
#include "Serialization.hpp"
#include "bls_types.hpp"
#include "opcodes.hpp"
#include <ostream>
#include <boost/archive/binary_oarchive.hpp>
#include <ranges>

template<typename T, typename U>
concept RangeOf = std::ranges::range<T> && std::is_same_v<std::ranges::range_value_t<T>, U>;

class BytecodeSerializer {
    public:
        BytecodeSerializer(std::ostream& stream) : stream(stream), oa(stream, boost::archive::archive_flags::no_header) { }
        void writeMetadata(std::unordered_map<std::string, std::pair<uint16_t, std::vector<std::string>>>& functionSymbols);
        void writeHeader(RangeOf<TaskDescriptor> auto&& taskDescriptors);
        void writeLiteralPool(RangeOf<BlsType> auto&& literals);
        void writeLiteralPool(std::unordered_map<BlsType, uint8_t>& literalMap);
        void writeInstruction(INSTRUCTION& instruction);

    private:
        std::ostream& stream;
        boost::archive::binary_oarchive oa;
};

inline void BytecodeSerializer::writeMetadata(std::unordered_map<std::string, std::pair<uint16_t, std::vector<std::string>>>& functionSymbols) {
    uint32_t metadataEnd = 0;
    stream.write(reinterpret_cast<const char *>(&metadataEnd), sizeof(metadataEnd));
    std::unordered_map<uint16_t, std::pair<std::string, std::vector<std::string>&>> functionMetadata;
    for (auto&& [name, metadata] : functionSymbols) {
        functionMetadata.emplace(metadata.first, std::make_pair(name, std::ref(metadata.second)));
    }
    oa << functionMetadata;
    metadataEnd = stream.tellp();
    stream.seekp(0);
    stream.write(reinterpret_cast<const char *>(&metadataEnd), sizeof(metadataEnd));
    stream.seekp(metadataEnd);
}

inline void BytecodeSerializer::writeHeader(RangeOf<TaskDescriptor> auto&& taskDescriptors) {
    uint16_t descriptorCount = taskDescriptors.size();
    stream.write(reinterpret_cast<const char *>(&descriptorCount), sizeof(descriptorCount));
    for (auto&& desc : taskDescriptors) {
        oa << desc;
    }
}

inline void BytecodeSerializer::writeLiteralPool(RangeOf<BlsType> auto&& literals) {
    uint16_t poolSize = literals.size();
    stream.write(reinterpret_cast<const char *>(&poolSize), sizeof(poolSize));
    for (auto&& literal : literals) {
        oa << literal;
    }
}

inline void BytecodeSerializer::writeLiteralPool(std::unordered_map<BlsType, uint8_t>& literalMap) {
    auto literals = std::views::keys(literalMap);
    std::vector<BlsType> orderedLiterals(literals.begin(), literals.end());
    std::sort(orderedLiterals.begin(), orderedLiterals.end(), [&](const auto& a, const auto& b) {
        return literalMap.at(a) < literalMap.at(b);
    });
    writeLiteralPool(orderedLiterals);
}

inline void BytecodeSerializer::writeInstruction(INSTRUCTION& instruction) {
    switch (instruction.opcode) {
        #define OPCODE_BEGIN(code) \
        case OPCODE::code: { \
            auto& resolvedInstruction [[ maybe_unused ]] = reinterpret_cast<INSTRUCTION::code&>(instruction); \
            stream.write(reinterpret_cast<const char *>(&instruction.opcode), sizeof(OPCODE));
        #define ARGUMENT(arg, type) \
            type& arg = resolvedInstruction.arg; \
            stream.write(reinterpret_cast<const char *>(&arg), sizeof(type));
        #define OPCODE_END(code, args...) \
            break; \
        } 
        #include "include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END
        default:
        break;
    }
}