#pragma once

#include <concepts>
#include <cstdio>
#include <string>
#include <sys/fcntl.h>
#include <unordered_map>

namespace Device {
    #define DEVTYPE_BEGIN(name) \
    class name;
    #define ATTRIBUTE(...)
    #define DEVTYPE_END
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
}

/* 
    enforces driver api adherence and collects all declarations for variant AbstractDevice creation
    might be possible to automate with modules but for now this will suffice
    until support for modules improves in clang and cmake
*/

#include "include/TIMER_TEST.hpp"
#include "include/LINE_WRITER.hpp"
#include "include/READ_FILE.hpp"
#include "include/FILE_LOG.hpp"
#include "include/LIGHT.hpp"
#include "include/BUTTON.hpp"
#include "include/READ_FILE_POLL.hpp"
#include "include/KEYBOARD.hpp"
#include "include/MOUSE.hpp"
#include "include/PWM_LED.hpp"
#include "include/ACTIVE_BUZZER.hpp"
#include "include/PASSIVE_BUZZER.hpp"
#include "include/RGB_LED.hpp"
#include "include/POTENTIOMETER.hpp"
#include "include/THERMISTOR.hpp"
#include "include/PHOTORESISTOR.hpp"
#include "include/FN_JOYSTICK.hpp"
#include "include/FN_SERVO.hpp"
#include "include/DC_MOTOR.hpp"
#include "include/STEP_MOTOR.hpp"
#include "include/DHT11.hpp"
#include "include/LCD_SCREEN.hpp"
#include "include/MOTION_SENSOR.hpp"
#include "include/ULTRASONIC.hpp"



template<typename T>
concept DeviceDriver = requires (T device, std::unordered_map<std::string, std::string>& config, DynamicMessage& dmsg) {
    { device.init(config) } -> std::same_as<void>;
    { device.processStates(dmsg) } -> std::same_as<void>;
    { device.transmitStates(dmsg) } -> std::same_as<void>;
};

#define DEVTYPE_BEGIN(name) \
static_assert(boost::is_complete<Device::name>::value, "Device::" #name " must have a complete class definition (perhaps you made a typo or forgot to include the header here?)."); \
static_assert(std::derived_from<Device::name, DeviceCore<TypeDef::name>>, "Device::" #name " must inherit from DeviceCore<TypeDef::" #name ">"); \
static_assert(DeviceDriver<Device::name>, "Device::" #name " must implement init(), processStates(), and transmitStates()");
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END
