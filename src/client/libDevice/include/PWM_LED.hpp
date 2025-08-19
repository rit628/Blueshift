#pragma once
#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

namespace Device{
    class PWM_LED : public DeviceCore<TypeDef::PWM_LED>{
        private: 
            uint8_t PIN;
        
        public: 
            ~PWM_LED();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}