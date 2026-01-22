
#ifdef __RPI64__

#include <exception>
#include <stdexcept>
#include "DynamicMessage.hpp"
#include <memory>
#include "include/STEP_MOTOR.hpp"

using namespace Device;

void STEP_MOTOR::moveOneStep(bool cw){
   
    for(int i = 0; i < 4; i++){
        for(int j = 0 ; j < 4; j++){
            if(cw){
                gpioWrite(this->portList[j], CWStep[i] == (1<<i) ? 1 : 0);
            }
            else{
                gpioWrite(this->portList[j], CCWStep[i] == (1<<i) ? 1 : 0);
            }
        }
        gpioDelay(this->ms_delay * 1000);
    }
}

void STEP_MOTOR::moveNSteps(bool cw, int steps){
    for(int i = 0; i < steps; i++){
        moveOneStep(cw);
    }
}


void STEP_MOTOR::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(states);
    bool cw = this->states.cw;
    int mv_steps = this->states.move_steps;
    moveNSteps(cw, mv_steps);
}

void STEP_MOTOR::init(std::unordered_map<std::string, std::string> &config){
    this->portList = {(uint8_t)stoi(config["PINA"]), (uint8_t)stoi(config["PINB"]), 
        (uint8_t)stoi(config["PINC"]), (uint8_t)stoi(config["PIND"])};

    if(gpioInitialise() < 0){
        std::cout<<"GPIO failed to iniitalize"<<std::endl;
        throw std::invalid_argument("Device Init failed!");
    }

    for(auto port : this->portList){
        gpioSetMode(port, PI_OUTPUT);
    }

    addGPIOIWatch(this->portList[0], [](int, int, uint32_t){return true;});
     
}


void STEP_MOTOR::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states);
}

STEP_MOTOR::~STEP_MOTOR(){ 

}

#endif