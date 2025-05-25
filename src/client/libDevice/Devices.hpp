#pragma once

#include <concepts>
#include <cstdio>
#include <sys/fcntl.h>

// namespace Device {
//     #define DEVTYPE_BEGIN(name) \
//     class name;
//     #define ATTRIBUTE(...)
//     #define DEVTYPE_END
//     #include "DEVTYPES.LIST"
//     #undef DEVTYPE_BEGIN
//     #undef ATTRIBUTE
//     #undef DEVTYPE_END
// };

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

// #define DEVTYPE_BEGIN(name) \
// static_assert(std::derived_from<Device::name, AbstractDevice>, #name " must inherit from AbstractDevice");
// #define ATTRIBUTE(...)
// #define DEVTYPE_END
// #include "DEVTYPES.LIST"
// #undef DEVTYPE_BEGIN
// #undef ATTRIBUTE
// #undef DEVTYPE_END
