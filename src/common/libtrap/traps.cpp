#include "include/traps.hpp"
#include "libtypes/bls_types.hpp"
#include <variant>
#include <vector>
#include <boost/range/iterator_range_core.hpp>

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

#define TRAP_BEGIN(name, ...) \
    BlsType BlsTrap::exec__##name(std::vector<BlsType> args) { \
        using argnum [[ maybe_unused ]] = Detail::name::ARGNUM; \
        return BlsTrap::name(
            #define VARIADIC(...) \
            args,
            #define ARGUMENT(argName, argType) \
            std::get<converted_t<argType>>(args[argnum::argName]),
            #define TRAP_END \
            0 \
        ); \
    }
#include "include/TRAPS.LIST"
#undef TRAP_BEGIN
#undef VARIADIC
#undef ARGUMENT
#undef TRAP_END