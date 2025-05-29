#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"

template<>
class Device<TypeDef::MOTOR> : public DeviceCore {
    private:
        TypeDef::MOTOR states;
        int speed;
        int in1_pin, in2_pin, pwm_pin;
        unsigned pwm_range;
        
    public:
        ~Device();
        void processStates(DynamicMessage &dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
        void apply_speed(int speed);
};
