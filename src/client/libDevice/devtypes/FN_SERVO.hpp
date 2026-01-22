#pragma once
#include "DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device{
    class FN_SERVO : public DeviceCore<TypeDef::FN_SERVO>{
        private: 
            uint8_t SERVO_PIN;
            const int servo_min_pulse = 500;
            const int servo_max_pulse = 2500;

            int getPulseFromAngle(int angle){
                if(angle < 0) angle = 0;
                if(angle > 180) angle = 180;
                return (servo_min_pulse) + (servo_max_pulse - servo_min_pulse) * angle/180;
            }

        public: 
            ~FN_SERVO();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}