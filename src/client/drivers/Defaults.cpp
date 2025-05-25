#include "Devices.hpp"
#include <stdexcept>

/* define default implementations for driver functions if devtype not supported on controller */

#define DEVTYPE_BEGIN(name) \
__attribute__ ((weak)) void Device<TypeDef::name>::set_ports(std::unordered_map<std::string, std::string> &srcs) { \
    throw std::runtime_error("DEVTYPE " #name " NOT SUPPORTED ON " CONTROLLER_TARGET " CONTROLLERS"); \
}
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END

#define DEVTYPE_BEGIN(name) \
__attribute__ ((weak)) void Device<TypeDef::name>::proc_message(DynamicMessage &dmsg) { \
    throw std::runtime_error("DEVTYPE " #name " NOT SUPPORTED ON " CONTROLLER_TARGET " CONTROLLERS"); \
}
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END

#define DEVTYPE_BEGIN(name) \
__attribute__ ((weak)) void Device<TypeDef::name>::read_data(DynamicMessage &dmsg) { \
    throw std::runtime_error("DEVTYPE " #name " NOT SUPPORTED ON " CONTROLLER_TARGET " CONTROLLERS"); \
}
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END

#define DEVTYPE_BEGIN(name) \
__attribute__ ((weak)) Device<TypeDef::name>::~Device() { }
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END