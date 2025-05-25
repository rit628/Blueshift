#ifdef __RPI64__

#include "../../include/MOTOR.hpp"
#include <pigpio.h>

void Device<TypeDef::MOTOR>::set_ports(std::unordered_map<std::string, std::string> &src)
{
    in1_pin  = std::stoi(src.at("IN1"));
    in2_pin = std::stoi(src.at("IN2"));
    pwm_pin = std::stoi(src.at("PWM"));
    pwm_range = std::stoi(src["PWM_RANGE"]);
    if(gpioInitialise() == PI_INIT_FAILED) 
    {
        std::cerr << "GPIO setup failed" << std::endl;
        return;
    }

    // Direction pins are outputs
    gpioSetMode(in1_pin, PI_OUTPUT);
    gpioSetMode(in2_pin, PI_OUTPUT);

    // PWM pin
    gpioSetMode(pwm_pin, PI_OUTPUT);
    gpioSetPWMrange(pwm_pin, pwm_range);
    // Optional: set a default PWM frequency (Hz)
    gpioSetPWMfrequency(pwm_pin, 1000);
}

void Device<TypeDef::MOTOR>::proc_message(DynamicMessage &dmsg)
{
    dmsg.unpackStates(states);

}

void Device<TypeDef::MOTOR>::read_data(DynamicMessage &dmsg)
{
    dmsg.packStates(states);
    apply_speed(states.speed);
}

void Device<TypeDef::MOTOR>::apply_speed(int speed)
{
    if(speed > 100) speed = 100;
        if(speed < -100) speed = -100;

        if(speed == 0) 
        {
            // brake / both low
            gpioWrite(in1_pin, PI_OFF);
            gpioWrite(in2_pin, PI_OFF);
            gpioPWM(pwm_pin, 0);
        }
        else if(speed > 0) 
        {
            // forward
            gpioWrite(in1_pin, PI_ON);
            gpioWrite(in2_pin, PI_OFF);
            // scale 0–100 to 0–pwm_range
            int duty = (speed * pwm_range) / 100;
            gpioPWM(pwm_pin, duty);
        }
        else 
        {
            // backward
            gpioWrite(in1_pin, PI_OFF);
            gpioWrite(in2_pin, PI_ON);
            int duty = ((-speed) * pwm_range) / 100;
            gpioPWM(pwm_pin, duty);
        }
}

Device<TypeDef::MOTOR>::~Device() 
{
    gpioTerminate();
}

#endif