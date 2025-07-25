#ifdef __RPI64__

#include <stdexcept>
#include "libDM/DynamicMessage.hpp"
#include <memory>
#include "include/FN_SERVO.hpp"


using namespace Device;


void FN_SERVO::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(this->states);
    int pulseWidth = this->getPulseFromAngle(this->states.angle);
    gpioServo(this->SERVO_PIN, pulseWidth);
}

void FN_SERVO::init(std::unordered_map<std::string, std::string> &config){
    this->SERVO_PIN = std::stoi(config["PIN"]);

    if(gpioInitialise() < 0){
        std::cout<<"GPIO failed to iniitalize"<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    gpioSetMode(this->SERVO_PIN, PI_OUTPUT);
    addGPIOIWatch(this->SERVO_PIN, [](int, int, uint32_t) -> bool { return false; });
}


void FN_SERVO::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states);
}

FN_SERVO::~FN_SERVO(){
    gpioTerminate();
}

#endif