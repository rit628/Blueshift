#include "traps.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "libEM/EM.hpp"
#include "libtype/bls_types.hpp"
#include <chrono>
#include <concepts>
#include <cstdint>
#include <sstream>
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
    // use a stringstream for now for better formatted output; eventually overloads will allow for std::to_string
    std::stringstream out;
    out << input;
    return out.str();
}

std::string Impl::getTrigger(BytecodeProcessor* vm) {
    if (!vm) return "";
    return vm->ownerUnit->TriggerName;
}

std::string Impl::time(BytecodeProcessor* vm) {
    std::time_t result = std::time(nullptr);
    return std::asctime(std::localtime(&result));
}


std::monostate Impl::disableTrigger(std::string triggerID, BytecodeProcessor* vm){
    std::cout<<"DISABLING TRIGGER: "<<triggerID<<std::endl;
    return std::monostate();
}
