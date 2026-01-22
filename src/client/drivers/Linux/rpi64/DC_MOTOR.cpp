#ifdef __RPI64__

#include <exception>
#include <stdexcept>
#include "DynamicMessage.hpp"
#include <memory>
#include "include/DC_MOTOR.hpp"

using namespace Device;

void DC_MOTOR::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(states);

    bool p1 = states.pin1 > 0;
    bool p2 = states.pin2 > 0;
    if(states.pwr > 255) states.pwr = 255;
    if(states.pwr < 0) states.pwr = 0;

    gpioWrite(this->cw_pin1, p1);
    gpioWrite(this->ccw_pin2, p2);
    gpioPWM(enablePin, states.pwr);
}

void DC_MOTOR::init(std::unordered_map<std::string, std::string> &config){
    this->cw_pin1 = std::stoi(config["PINA"]);
    this->ccw_pin2 = std::stoi(config["PINB"]);
    this->enablePin = std::stoi(config["ENABLE"]);

    if(gpioInitialise() < 0){
        std::cout<<"GPIO failed to iniitalize"<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    gpioSetMode(this->cw_pin1, PI_OUTPUT);
    gpioSetMode(this->ccw_pin2, PI_OUTPUT);
    gpioSetMode(this->enablePin, PI_OUTPUT);
}


void DC_MOTOR::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states);
}

DC_MOTOR::~DC_MOTOR(){
    // Turn off the motor when object is destroyed: 
    gpioWrite(this->cw_pin1, 0);
    gpioWrite(this->ccw_pin2, 0);
    gpioPWM(enablePin, 0);
}


#endif