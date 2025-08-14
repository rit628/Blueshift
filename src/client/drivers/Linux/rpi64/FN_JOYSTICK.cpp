#ifdef __RPI64__

#include <exception>
#include <stdexcept>
#include "libDM/DynamicMessage.hpp"
#include <memory>
#include "include/FN_JOYSTICK.hpp"


using namespace Device;


void FN_JOYSTICK::processStates(DynamicMessage &dmsg){
    // Read only
}

void FN_JOYSTICK::init(std::unordered_map<std::string, std::string> &config){
    this->xaxis_adc = std::stoi(config["XAXIS"]);
    this->yaxis_adc = std::stoi(config["YAXIS"]);
    this->zaxis_pin = std::stoi(config["ZAXIS"]);

    if(gpioInitialise() < 0){
        std::cout<<"GPIO failed to iniitalize"<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    if(!this->adc->isValid()){
        std::cout<<"INVALID ADC "<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    gpioSetMode(this->zaxis_pin, PI_INPUT);
    gpioSetPullUpDown(this->zaxis_pin, PI_PUD_UP);
}


void FN_JOYSTICK::transmitStates(DynamicMessage &dmsg){
    states.x = this->adc->readByte(this->xaxis_adc);
    states.y = this->adc->readByte(this->yaxis_adc);
    states.z = this->adc->readByte(this->zaxis_pin);

    dmsg.packStates(states);
}

FN_JOYSTICK::~FN_JOYSTICK(){
    
}

#endif