#pragma once

#include <concepts>
#include <cstdio>
#include <string>
#include <sys/fcntl.h>
#include <unordered_map>

/* 
    enforces driver api adherence and collects all declarations for variant AbstractDevice creation
    might be possible to automate with modules but for now this will suffice
    until support for modules improves in clang
*/
#include "include/TIMER_TEST.hpp"
#include "include/LINE_WRITER.hpp"
#include "include/LIGHT.hpp"
#include "include/BUTTON.hpp"
#include "include/MOTOR.hpp"
#include "include/READ_FILE_POLL.hpp"

template<typename T>
concept DeviceDriver = requires (T device, std::unordered_map<std::string, std::string>& config, DynamicMessage& dmsg) {
    { device.init(config) } -> std::same_as<void>;
    { device.processStates(dmsg) } -> std::same_as<void>;
    { device.transmitStates(dmsg) } -> std::same_as<void>;
};

#define DEVTYPE_BEGIN(name) \
static_assert(std::derived_from<Device<TypeDef::name>, DeviceCore>, "Device<TypeDef::" #name "> must inherit from DeviceCore"); \
static_assert(DeviceDriver<Device<TypeDef::name>>, "Device<TypeDef::" #name "> must implement init(), processStates(), and transmitStates()");
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END
