#pragma once
#include "libbytecode/bytecode_processor.hpp"
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

    enum class MCALLNUM : uint16_t {
        #define METHOD_BEGIN(name, objType, ...) \
        objType##__##name,
        #define ARGUMENT(...)
        #define METHOD_END
        #include "libtype/include/LIST_METHODS.LIST"
        #include "libtype/include/MAP_METHODS.LIST"
        #undef METHOD_BEGIN
        #undef ARGUMENT
        #undef METHOD_END
        COUNT
    };

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

        #define METHOD_BEGIN(name, objType, ...) \
        namespace objType##__##name { \
            enum ARGNUM : uint8_t {
            #define ARGUMENT(argName, ...) \
                argName,
            #define METHOD_END \
                COUNT \
            }; \
        }
        #include "libtype/include/LIST_METHODS.LIST"
        #include "libtype/include/MAP_METHODS.LIST"
        #undef METHOD_BEGIN
        #undef ARGUMENT
        #undef METHOD_END

        #define TRAP_BEGIN(trapName, retType...) \
        namespace trapName { \
            static BlsType returnType = resolveBlsType<retType>(resolved_t<retType>()); \
            static std::initializer_list<BlsType> parameterTypes = {
                #define VARIADIC(...)
                #define ARGUMENT(argName, argType...) \
                resolveBlsType<argType>(resolved_t<argType>()),
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
    
    struct Impl {
        #define TRAP_BEGIN(name, returnType...) \
        static resolved_t<returnType> name(
        #define VARIADIC(argName) \
            std::vector<BlsType> argName,
        #define ARGUMENT(argName, argType...) \
            resolved_t<argType> argName,
        #define TRAP_END \
        BytecodeProcessor* vm = nullptr);
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END
    };


    template<CALLNUM T>
    BlsType executeTrap(std::vector<BlsType> args, BytecodeProcessor* vm = nullptr) {
        #define TRAP_BEGIN(name, ...) \
        if constexpr (T == CALLNUM::name) { \
            using argnum [[ maybe_unused ]] = Detail::name::ARGNUM; \
            return BlsTrap::Impl::name(
                #define VARIADIC(...) \
                args,
                #define ARGUMENT(argName, argType...) \
                resolveBlsType<argType>(std::move(args[argnum::argName])),
                #define TRAP_END \
                vm \
            ); \
        }
        #include "include/TRAPS.LIST"
        #undef TRAP_BEGIN
        #undef VARIADIC
        #undef ARGUMENT
        #undef TRAP_END
    }

    template<MCALLNUM T>
    BlsType executeMTRAP(BlsType object, std::vector<BlsType> args) {
        using list = VectorDescriptor;
        using map = MapDescriptor;
        
        auto operable = std::get<std::shared_ptr<HeapDescriptor>>(object);
        #define METHOD_BEGIN(name, type, ...) \
        if constexpr (T == MCALLNUM::type##__##name) { \
            using argnum [[ maybe_unused ]] = Detail::type##__##name::ARGNUM; \
            return std::dynamic_pointer_cast<type>(operable)->name(
            #define ARGUMENT(argName, typeArgIdx, type...) \
                resolveBlsType<type>(std::move(args[argnum::argName])),
            #define METHOD_END \
            0); \
        }
        #include "libtype/include/LIST_METHODS.LIST"
        #include "libtype/include/MAP_METHODS.LIST"
        #undef METHOD_BEGIN
        #undef ARGUMENT
        #undef METHOD_END
    }

}