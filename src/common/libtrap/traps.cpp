#include "traps.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "libtype/bls_types.hpp"
#include <chrono>
#include <concepts>
#include <cstdint>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include <boost/range/iterator_range_core.hpp>

using namespace BlsTrap;

std::monostate Impl::print(std::vector<BlsType> values, BytecodeProcessor* vm) {
    if (values.size() > 0) {
        std::cout << values.at(0) << std::flush;
        for (auto&& arg : boost::make_iterator_range(values.begin()+1, values.end())) {
            std::cout << " " << arg << std::flush;
        }
        std::cout << std::endl;
    }
    return std::monostate();
}

std::monostate Impl::println(std::vector<BlsType> values, BytecodeProcessor* vm) {
    for (auto&& value : values) {
        std::cout << value << std::endl;
    }
    return std::monostate();
}

std::monostate Impl::sleep(int64_t seconds, BytecodeProcessor* vm) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    return std::monostate();
}

int64_t Impl::stoi(std::string input, BytecodeProcessor* vm) {
    return std::stoi(input);
}

std::string Impl::toString(double input, BytecodeProcessor* vm) {
    return std::to_string(input);
}

std::string Impl::getTrigger(BytecodeProcessor* vm) {
    if (!vm) {
        return "";
    }
    // get triggers from master
    return "";    
}