#ifdef __RPI64__

#include <exception>
#include <stdexcept>
#include "DynamicMessage.hpp"
#include <memory>
#include "PHOTORESISTOR.hpp"


using namespace Device;


void PHOTORESISTOR::processStates(DynamicMessage &dmsg){
    // Read only
}


void PHOTORESISTOR::init(std::unordered_map<std::string, std::string> &config){
    this->ADC_CHANNEL = std::stoi(config["ADC_CHANNEL"]);

    if(gpioInitialise() < 0){
        std::cout<<"GPIO failed to iniitalize"<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    if(!this->adc->isValid()){
        std::cout<<"INVALID ADC "<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }
}


void PHOTORESISTOR::transmitStates(DynamicMessage &dmsg){
    states.intensity = this->adc->readByte(this->ADC_CHANNEL);
    dmsg.packStates(states);
}

PHOTORESISTOR::~PHOTORESISTOR(){
    
}


#endif