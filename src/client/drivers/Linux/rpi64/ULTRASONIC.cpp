#ifdef __RPI64__

#include "include/ULTRASONIC.hpp"
#include "DynamicMessage.hpp"
#include <pigpio.h>

using namespace Device;

// gpio tick returns the absolute microseconds since the init of the gpio
int ULTRASONIC::pulseIn(int pin, int level, int timeout){
    uint32_t startTick = gpioTick();
    uint32_t nowTick;

    while(gpioRead(pin) != level){
        nowTick = gpioTick();
        if((nowTick - startTick) >timeout) return 0;
    }

    // Reset the timer when the gpio is at the desired level
    uint32_t pulseStart = gpioTick();
    
    while(gpioRead(pin) == level){
        nowTick = gpioTick();
        if((nowTick - pulseStart) > timeout) return 0;
    }

    uint32_t pulseEnd = gpioTick();
    return (pulseEnd - pulseStart);
}


float ULTRASONIC::getSonar(){
    uint32_t pingTime;
    float distance;

    gpioWrite(this->TRIG_PIN, 0);
    gpioDelay(10);
    gpioWrite(this->TRIG_PIN, 0);

    pingTime = pulseIn(this->ECHO_PIN, 1, this->timeout);
    distance = (float)pingTime * 340.0 / 2.0 / 10000.0;
    return distance;
}


void ULTRASONIC::processStates(DynamicMessage &dmsg){
    // Read only
}

void ULTRASONIC::init(std::unordered_map<std::string, std::string> &config){
    this->TRIG_PIN = std::stoi(config.at("TRIG_PIN"));
    this->ECHO_PIN = std::stoi(config.at("ECHO_PIN"));

    if(gpioInitialise() < 0){
        throw std::runtime_error("Failed to initalize gpio for ULTRASONIC");
    }

    if(config.contains("MAX_DISTANCE")){
        this->MAX_DISTANCE = std::stoi(config.at("MAX_DISTANCE"));
    }

    // Ill figure this out later
    this->timeout = this->MAX_DISTANCE * 60;
}

void ULTRASONIC::transmitStates(DynamicMessage &dmsg){
    this->states.distance = getSonar();
    dmsg.packStates(this->states);
}

#endif