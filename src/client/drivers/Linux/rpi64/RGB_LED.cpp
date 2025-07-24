#ifdef __RPI64__

#include "include/RGB_LED.hpp"
#include "libDM/DynamicMessage.hpp"
#include <pigpio.h>

using namespace Device;

void RGB_LED::processStates(DynamicMessage &dmsg){
    dmsg.packStates(this->states);

    // Get the greatest value for each device
    states.red_val =  states.red_val < 255 ? states.red_val : 255;
    states.green_val =  states.green_val < 255 ? states.green_val : 255;
    states.blue_val =  states.blue_val < 255 ? states.blue_val : 255;

    // Sets the gpio values of the RGB_LED
    gpioPWM(this->RED_PIN, states.red_val);
    gpioPWM(this->GREEN_PIN, states.green_val);
    gpioPWM(this->BLUE_PIN, states.blue_val);

}

void RGB_LED::init(std::unordered_map<std::string, std::string> &config){
    this->RED_PIN = std::stoi(config["RED_PIN"]);
    this->GREEN_PIN = std::stoi(config["GREEN_PIN"]); 
    this->BLUE_PIN = std::stoi(config["BLUE_PIN"]);

    if(gpioInitialise() == PI_INIT_FAILED){
        std::cerr<<"Could not find device"<<std::endl;
        return; 
    }

    gpioSetMode(this->RED_PIN, PI_OUTPUT);
    gpioSetMode(this->GREEN_PIN, PI_OUTPUT);
    gpioSetMode(this->BLUE_PIN, PI_OUTPUT);

    // Only write interrupt on one device so 
    addGPIOIWatch(this->RED_PIN, [](int, int, uint32_t){return true;});

}

void RGB_LED::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(this->states);
}

RGB_LED::~RGB_LED(){
    gpioTerminate();
}

#endif