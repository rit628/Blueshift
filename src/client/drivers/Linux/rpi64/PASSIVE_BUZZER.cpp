#ifdef __RPI64__

#include "include/PASSIVE_BUZZER.hpp"
#include "libDM/DynamicMessage.hpp"
#include <pigpio.h>
#include <unordered_map>

using namespace Device; 

void PASSIVE_BUZZER::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(this->states);
    gpioHardwarePWM(this->PIN, states.tone, states.duty_cycle);
}

void PASSIVE_BUZZER::init(std::unordered_map<std::string, std::string> &config){
    this->PIN = std::stoi(config["PIN"]);
    if(gpioInitialise() == PI_INIT_FAILED){
        std::cerr<<"Could not find device"<<std::endl;
        return; 
    }

    gpioSetMode(this->PIN, PI_OUTPUT);
    addGPIOIWatch(this->PIN, [](int, int, uint32_t){return true;});
}

void PASSIVE_BUZZER::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states);
}

PASSIVE_BUZZER::~PASSIVE_BUZZER(){
    gpioTerminate();
}

#endif