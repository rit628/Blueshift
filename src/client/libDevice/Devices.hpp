#pragma once

#include <concepts>
#include <cstdio>
#include <string>
#include <sys/fcntl.h>
#include <unordered_map>

/* 
    might be possible to automate with modules but for now this will suffice
    until support for modules improves in clang
*/
#include "drivers/include/TIMER_TEST.hpp"
#include "drivers/include/LINE_WRITER.hpp"
#include "drivers/include/LIGHT.hpp"
#include "drivers/include/BUTTON.hpp"
#include "drivers/include/MOTOR.hpp"
#include "drivers/include/READ_FILE_POLL.hpp"

template<typename T>
concept DeviceDriver = requires (T device, std::unordered_map<std::string, std::string>& srcs, DynamicMessage& dmsg) {
    { device.set_ports(srcs) } -> std::same_as<void>;
    { device.proc_message(dmsg) } -> std::same_as<void>;
    { device.read_data(dmsg) } -> std::same_as<void>;
};

#define DEVTYPE_BEGIN(name) \
static_assert(std::derived_from<Device<TypeDef::name>, DeviceCore>, "Device<TypeDef::" #name "> must inherit from DeviceCore"); \
static_assert(DeviceDriver<Device<TypeDef::name>>, "Device<TypeDef::" #name "> must implement set_ports(), proc_message(), and read_data()");
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END
