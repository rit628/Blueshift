#ifdef __RPI64__

#include "include/PWM_LED.hpp"
#include "libDM/DynamicMessage.hpp"
#include <cstdint>
#include <pigpio.h>

using namespace Device;

void PWM_LED::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(states);
    if(states.intensity > 255){
        states.intensity = 255;
    }
    gpioPWM(this->PIN ,states.intensity);
}

void PWM_LED::init(std::unordered_map<std::string, std::string> &config){
    this->PIN = std::stoi(config["PIN"]);
    if(gpioInitialise() == PI_INIT_FAILED){
        std::cerr<<"Could not find device"<<std::endl;
    }

    gpioSetMode(this->PIN, PI_OUTPUT);
    addGPIOIWatch(this->PIN, [](int, int, uint32_t){return false;});
}


void PWM_LED::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states);
}

PWM_LED::~PWM_LED(){
    
}

#endif