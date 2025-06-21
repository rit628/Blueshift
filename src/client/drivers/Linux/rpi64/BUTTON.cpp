#ifdef __RPI64__

#include "include/BUTTON.hpp"
#include <pigpio.h>

using namespace Device;

bool BUTTON::handleInterrupt(int gpio, int level, uint32_t tick) {
    // static auto DEBOUNCE_TIME = std::chrono::microseconds(10);
    if (level == RISING_EDGE) {
        states.pressed = true;
    }
    else if (level == FALLING_EDGE) {
        states.pressed = false;
    }
    else {
        return false;
    }
    // auto currPress = std::chrono::high_resolution_clock::now();
    // bool ret = (currPress - lastPress) > DEBOUNCE_TIME;
    // lastPress = currPress;
    // return ret;
    return true;

}

void BUTTON::init(std::unordered_map<std::string, std::string> &config) {
    this->PIN = std::stoi(config.at("PIN"));
    this->lastPress = std::chrono::high_resolution_clock::now();
    if (gpioInitialise() == PI_INIT_FAILED) 
    {
        std::cerr << "GPIO setup failed" << std::endl;
        return;
    }
    gpioSetMode(this->PIN, PI_INPUT);
    gpioSetPullUpDown(this->PIN, PI_PUD_UP);

    auto bound = std::bind(&BUTTON::handleInterrupt, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); 

    addGPIOIWatch(this->PIN, bound);
}

void BUTTON::processStates(DynamicMessage &dmsg) {
    // should do nothing
    dmsg.unpackStates(states);
}

void BUTTON::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

BUTTON::~BUTTON() {
    gpioTerminate();
}

#endif