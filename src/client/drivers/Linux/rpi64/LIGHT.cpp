#ifdef __RPI64__

#include "include/LIGHT.hpp"
#include <pigpio.h>

void Device<TypeDef::LIGHT>::proc_message(DynamicMessage &dmsg) {
    dmsg.unpackStates(states);
    if (states.on) {
        gpioWrite(this->PIN, PI_ON);
    }
    else {
        gpioWrite(this->PIN, PI_OFF);
    }
}

void Device<TypeDef::LIGHT>::set_ports(std::unordered_map<std::string, std::string> &src) {
    this->PIN = std::stoi(src["PIN"]);
    if (gpioInitialise() == PI_INIT_FAILED) {
            std::cerr << "GPIO setup failed" << std::endl;
            return;
    }
    gpioSetMode(this->PIN, PI_OUTPUT);
}

void Device<TypeDef::LIGHT>::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

Device<TypeDef::LIGHT>::~Device() {
    gpioTerminate();
}

#endif