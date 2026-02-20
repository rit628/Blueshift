#pragma once

#include <concepts>
#include <cstdio>
#include <string>
#include <sys/fcntl.h>
#include <unordered_map>

namespace Device {
    #define DEVTYPE_BEGIN(name, ...) \
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-private-field"

#include "devtypes/TIMER_TEST.hpp"
#include "devtypes/LINE_WRITER.hpp"
#include "devtypes/READ_FILE.hpp"
#include "devtypes/FILE_LOG.hpp"
#include "devtypes/READ_FILE_POLL.hpp"
#include "devtypes/LIGHT.hpp"
#include "devtypes/PWM_LED.hpp"
#include "devtypes/RGB_LED.hpp"
#include "devtypes/BUTTON.hpp"
#include "devtypes/ACTIVE_BUZZER.hpp"
#include "devtypes/PASSIVE_BUZZER.hpp"
#include "devtypes/POTENTIOMETER.hpp"
#include "devtypes/THERMISTOR.hpp"
#include "devtypes/PHOTORESISTOR.hpp"
#include "devtypes/FN_JOYSTICK.hpp"
#include "devtypes/FN_SERVO.hpp"
#include "devtypes/DC_MOTOR.hpp"
#include "devtypes/STEP_MOTOR.hpp"
#include "devtypes/DHT11.hpp"
#include "devtypes/LCD_SCREEN.hpp"
#include "devtypes/MOTION_SENSOR.hpp"
#include "devtypes/ULTRASONIC.hpp"
#include "devtypes/KEYBOARD.hpp"
#include "devtypes/MOUSE.hpp"
#include "devtypes/GAMEPAD.hpp"
#include "devtypes/HTTP_ENDPOINT.hpp"
#include "devtypes/TEXT_FILE.hpp"
#include "devtypes/AUDIO_PLAYER.hpp"

#pragma GCC diagnostic pop

template<typename T>
concept DeviceDriver = requires (T device, std::unordered_map<std::string, std::string>& config, DynamicMessage& dmsg) {
    { device.init(config) } -> std::same_as<void>;
    { device.processStates(dmsg) } -> std::same_as<void>;
    { device.transmitStates(dmsg) } -> std::same_as<void>;
};

#define DEVTYPE_BEGIN(name, ...) \
static_assert(boost::is_complete<Device::name>::value, "Device::" #name " must have a complete class definition (perhaps you made a typo or forgot to include the header here?)."); \
static_assert(std::derived_from<Device::name, DeviceCore<TypeDef::name>>, "Device::" #name " must inherit from DeviceCore<TypeDef::" #name ">"); \
static_assert(DeviceDriver<Device::name>, "Device::" #name " must implement init(), processStates(), and transmitStates()");
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END
