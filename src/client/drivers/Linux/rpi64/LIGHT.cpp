#ifdef __RPI64__

#include "include/LIGHT.hpp"
#include <pigpio.h>

void Device<TypeDef::LIGHT>::processStates(DynamicMessage &dmsg) {
    dmsg.unpackStates(states);
    if (states.on) {
        gpioWrite(this->PIN, PI_ON);
    }
    else {
        gpioWrite(this->PIN, PI_OFF);
    }
}

void Device<TypeDef::LIGHT>::init(std::unordered_map<std::string, std::string> &config) {
    this->PIN = std::stoi(config["PIN"]);
    if (gpioInitialise() == PI_INIT_FAILED) {
            std::cerr << "GPIO setup failed" << std::endl;
            return;
    }
    gpioSetMode(this->PIN, PI_OUTPUT);
}

void Device<TypeDef::LIGHT>::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

Device<TypeDef::LIGHT>::~Device() {
    gpioTerminate();
}

#endif