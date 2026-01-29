#pragma once
#include "Serialization.hpp"
#include "opcodes.hpp"
#include "bls_types.hpp"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <vector>
#include <boost/archive/binary_iarchive.hpp>

template<typename Derived = void, bool SkipMetadata = true>
class BytecodeProcessor {
    public:
        enum class SIGNAL : uint8_t {
            START,
            STOP,
            COUNT
        };

        /* default callback for dispatch() */
        [[ gnu::always_inline ]] static inline void nop(INSTRUCTION&, size_t, SIGNAL) noexcept { }

        void loadBytecode(std::istream& bytecode);
        void loadBytecode(const std::string& filename);
        void loadBytecode(const std::vector<char>& bytecode);
        template<std::invocable<INSTRUCTION&, size_t, SIGNAL> F = decltype(nop)>
        void dispatch(F&& preExecFunction = nop, F&& postExecFunction = nop);

    private:
        void readMetadata(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void readHeader(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void loadLiterals(std::istream& bytecode, boost::archive::binary_iarchive& ia);
        void loadInstructions(std::istream& bytecode);

    protected:
        /* use crtp for now until we upgrade to c++ 23 */
        BytecodeProcessor() = default;

        #define OPCODE_BEGIN(code) \
        void code(
        #define ARGUMENT(arg, type) \
        type arg,
        #define OPCODE_END(code, args...) \
        int = 0) { static_cast<Derived*>(this)->code(args); }
        #include "include/OPCODES.LIST"
        #undef OPCODE_BEGIN
        #undef ARGUMENT
        #undef OPCODE_END

        /* metadata */
        std::unordered_map<uint16_t, std::pair<std::string, std::vector<std::string>>> functionMetadata;
        /* header data */
        std::vector<TaskDescriptor> taskDescs;
        /* literal pool */
        std::vector<BlsType> literalPool;
        /* bytecode */
        std::vector<std::unique_ptr<INSTRUCTION>> instructions;
        /* execution data */
        size_t instruction = 0;
        SIGNAL signal = SIGNAL::START;
};

#include "bytecode_processor.tpp"