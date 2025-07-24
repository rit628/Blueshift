#pragma once
#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

namespace Device{
    class POTENTIOMETER : public DeviceCore<TypeDef::POTENTIOMETER>{
        private: 
            uint8_t ADC_CHANNEL;

        public: 
            ~POTENTIOMETER();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}