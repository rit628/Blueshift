#pragma once
#include "../DeviceCore.hpp"
#include "FN_SERVO.hpp"
#include "typedefs.hpp"

namespace Device{
    class FN_JOYSTICK : public DeviceCore<TypeDef::FN_JOYSTICK>{
        private: 
            uint8_t xaxis_adc;
            uint8_t yaxis_adc;
            uint8_t zaxis_pin;

        public: 
            ~FN_JOYSTICK();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}