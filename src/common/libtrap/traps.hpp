#pragma once
#include "libtype/bls_types.hpp"
#include "libtype/typedefs.hpp"
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace BlsTrap {

    using namespace TypeDef;
    
    enum class CALLNUM : uint16_t {
        #define TRAP_BEGIN(name, ...) \
        name,
        #define VARIADIC(...)
        #define ARGUMENT(...)
        #define TRAP_END
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END
        COUNT
    };

    constexpr CALLNUM getTrapCallNum(const std::string& name) {
        #define TRAP_BEGIN(trapName, ...) \
        if (name == #trapName) return CALLNUM::trapName;
        #define VARIADIC(...)
        #define ARGUMENT(...)
        #define TRAP_END
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END
        return CALLNUM::COUNT;
    }

    namespace Detail {

        #define TRAP_BEGIN(trapName, ...) \
        namespace trapName { \
            enum ARGNUM : uint8_t {
                #define VARIADIC(...)
                #define ARGUMENT(argName, ...) \
                argName,
                #define TRAP_END \
                COUNT \
            }; \
        }
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END

        #define TRAP_BEGIN(trapName, retType...) \
        namespace trapName { \
            static BlsType returnType = createBlsType(converted_t<retType>()); \
            static std::initializer_list<BlsType> parameterTypes = {
                #define VARIADIC(...)
                #define ARGUMENT(argName, argType...) \
                createBlsType(converted_t<argType>()),
                #define TRAP_END \
            }; \
        }
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END

        #define TRAP_BEGIN(trapName, returnType...) \
        namespace trapName { \
            static std::initializer_list<std::pair<const std::string, uint8_t>> parameterIndices = {
                #define VARIADIC(...)
                #define ARGUMENT(argName, argType...) \
                { #argName, ARGNUM::argName },
                #define TRAP_END \
            }; \
        }
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END

        template<typename T = void>
        struct variadic_helper : std::false_type {};
        template<>
        struct variadic_helper<bool> : std::true_type {};

        #define TRAP_BEGIN(trapName, returnType...) \
        namespace trapName { \
            constexpr bool variadic = Detail::variadic_helper<
            #define VARIADIC(...) \
            bool
            #define ARGUMENT(argName, argType...)
            #define TRAP_END \
            >::value; \
        }
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END

    }
    
    #define TRAP_BEGIN(name, returnType...) \
    resolved_t<returnType> name(
    #define VARIADIC(argName) \
        std::vector<BlsType> argName,
    #define ARGUMENT(argName, argType...) \
        resolved_t<argType> argName,
    #define TRAP_END \
    int = 0);
    #include "include/TRAPS.LIST"
    #undef TRAP_BEGIN
    #undef VARIADIC
    #undef ARGUMENT
    #undef TRAP_END

    template<CALLNUM T>
    BlsType executeTrap(std::vector<BlsType> args) {
        #define TRAP_BEGIN(name, ...) \
        if constexpr (T == CALLNUM::name) { \
            using argnum [[ maybe_unused ]] = Detail::name::ARGNUM; \
            return name(
                #define VARIADIC(...) \
                args,
                #define ARGUMENT(argName, argType) \
                resolveBlsType<argType>(std::move(args[argnum::argName])),
                #define TRAP_END \
                0 \
            ); \
        }
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END
    }

}