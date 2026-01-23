#ifdef __RPI64__

#include "ACTIVE_BUZZER.hpp"
#include <pigpio.h>

using namespace Device;

void ACTIVE_BUZZER::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(states);
    if(states.active){
        gpioWrite(this->PIN, 1);
    }
    else{
        gpioWrite(this->PIN, 0);
    }
}


void ACTIVE_BUZZER::init(std::unordered_map<std::string, std::string> &config){
    this->PIN = std::stoi(config["PIN"]);
    if(gpioInitialise() == PI_INIT_FAILED){
        std::cerr<<"Could not find device"<<std::endl;
        return; 
    }

    gpioSetMode(this->PIN, PI_OUTPUT);
    addGPIOIWatch(this->PIN, [](int, int, uint32_t){return true;});
}

void ACTIVE_BUZZER::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states);
}

ACTIVE_BUZZER::~ACTIVE_BUZZER(){
    
}

#endif