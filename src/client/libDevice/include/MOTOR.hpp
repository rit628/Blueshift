#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

namespace Device {
    class MOTOR : public DeviceCore<TypeDef::MOTOR> {
        private:
            int speed;
            int in1_pin, in2_pin, pwm_pin;
            unsigned pwm_range;
            
        public:
            ~MOTOR();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            void apply_speed(int speed);
    };
}