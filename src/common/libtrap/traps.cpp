#include "traps.hpp"
#include "libtype/bls_types.hpp"
#include <concepts>
#include <variant>
#include <vector>
#include <boost/range/iterator_range_core.hpp>

using namespace BlsTrap;

std::monostate BlsTrap::print(std::vector<BlsType> values, int) {
    if (values.size() > 0) {
        std::cout << values.at(0) << std::flush;
        for (auto&& arg : boost::make_iterator_range(values.begin()+1, values.end())) {
            std::cout << " " << arg << std::flush;
        }
        std::cout << std::endl;
    }
    return std::monostate();
}

std::monostate BlsTrap::println(std::vector<BlsType> values, int) {
    for (auto&& value : values) {
        std::cout << value << std::endl;
    }
    return std::monostate();
}