#ifdef __RPI64__
#pragma once
#include <exception>
#include <stdexcept>
#include "libDM/DynamicMessage.hpp"
#include <memory>
#include "include/POTENTIOMETER.hpp"


using namespace Device;


void POTENTIOMETER::processStates(DynamicMessage &dmsg){
    // Read only
}


void POTENTIOMETER::init(std::unordered_map<std::string, std::string> &config){
    this->ADC_CHANNEL = std::stoi(config["ADC_CHANNEL"]);

    if(gpioInitialise() < 0){
        std::cout<<"GPIO failed to iniitalize"<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    if(!this->adc){
        std::cout<<"WHAT! IS NULLPTR!"<<std::endl;
    }

    if(!this->adc->isValid()){
        std::cout<<"INVALID ADC "<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }
}


void POTENTIOMETER::transmitStates(DynamicMessage &dmsg){
    states.value = (int)this->adc->readByte(this->ADC_CHANNEL);
    dmsg.packStates(states);
}

POTENTIOMETER::~POTENTIOMETER(){
        
}


#endif