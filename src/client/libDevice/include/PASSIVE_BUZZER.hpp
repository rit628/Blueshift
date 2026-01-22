#pragma once
#include "../DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device{
    class  PASSIVE_BUZZER : public DeviceCore<TypeDef::PASSIVE_BUZZER>{
        private: 
            uint8_t PIN; 
        
        public: 
            ~PASSIVE_BUZZER();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}