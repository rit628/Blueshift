#ifdef __RPI64__

#include <exception>
#include <stdexcept>
#include "DynamicMessage.hpp"
#include <memory>
#include "include/THERMISTOR.hpp"


using namespace Device;


void THERMISTOR::processStates(DynamicMessage &dmsg){
    // Read only
}


void THERMISTOR::init(std::unordered_map<std::string, std::string> &config){
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


void THERMISTOR::transmitStates(DynamicMessage &dmsg){
    // some conversion stuff

    int readValue = this->adc->readByte(this->ADC_CHANNEL);
    float voltage = (float)readValue/255.0 * 3.3;
    float rt = 10 * voltage/(3.3 - voltage);
    float tempk = 1/(1/(273.15 + 25)) + log(rt/10)/3950.0;
    states.celsius = tempk - 273.15;

    dmsg.packStates(states);
}

THERMISTOR::~THERMISTOR(){
    
}


#endif