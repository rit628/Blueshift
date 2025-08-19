#pragma once
#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

namespace Device{
    class  ACTIVE_BUZZER : public DeviceCore<TypeDef::ACTIVE_BUZZER>{
        private: 
            uint8_t PIN; 
        
        public: 
            ~ACTIVE_BUZZER();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}