#pragma once

#include "../DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device {
    class ULTRASONIC : public DeviceCore<TypeDef::ULTRASONIC> {
        private:
            uint8_t TRIG_PIN;
            uint8_t ECHO_PIN;
            int MAX_DISTANCE = 220;
            int timeout;


            int pulseIn(int pin, int level, int timeout);
            float getSonar();



        public:
            ~ULTRASONIC();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}