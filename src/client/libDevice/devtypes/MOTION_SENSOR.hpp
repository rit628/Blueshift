#pragma once

#include "DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device {
    class MOTION_SENSOR : public DeviceCore<TypeDef::MOTION_SENSOR> {
        private:
            uint8_t PIN;
            
        public:
            ~MOTION_SENSOR();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            bool handleInterrupt(int gpio, int level, uint32_t tick);
    };
}