#pragma once
#include "../DeviceCore.hpp"
#include "typedefs.hpp"


namespace Device{
    class THERMISTOR : public DeviceCore<TypeDef::THERMISTOR>{
        private: 
            uint8_t ADC_CHANNEL;

        public: 
            ~THERMISTOR();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}