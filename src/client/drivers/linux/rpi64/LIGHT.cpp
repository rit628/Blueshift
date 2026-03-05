#ifdef __RPI64__

#include "LIGHT.hpp"
#include <cstdint>
#include <pigpio.h>

using namespace Device;

void LIGHT::processStates(DynamicMessage &dmsg) {
    dmsg.unpackStates(states);
    if (states.on) {
        gpioWrite(this->PIN, PI_ON);
    }
    else {
        gpioWrite(this->PIN, PI_OFF);
    }
}

void LIGHT::init(std::unordered_map<std::string, std::string> &config) {

    this->PIN = std::stoi(config["PIN"]);
    if (gpioInitialise() == PI_INIT_FAILED) {
            std::cerr << "GPIO setup failed" << std::endl;
            return;
    }
    gpioSetMode(this->PIN, PI_OUTPUT);
    addGPIOIWatch(this->PIN, [](int, int, uint32_t) -> bool { return true; });
}

void LIGHT::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

LIGHT::~LIGHT() {
    gpioWrite(this->PIN, PI_OFF);
}

#endif