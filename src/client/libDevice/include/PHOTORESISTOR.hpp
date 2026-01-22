#pragma once
#include "../DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device{
    class PHOTORESISTOR : public DeviceCore<TypeDef::PHOTORESISTOR>{
        private: 
            uint8_t ADC_CHANNEL;

        public: 
            ~PHOTORESISTOR();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}