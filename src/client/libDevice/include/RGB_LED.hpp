#pragma once
#include "../DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device{
    class RGB_LED : public DeviceCore<TypeDef::RGB_LED>{
        private: 
            uint8_t RED_PIN;
            uint8_t BLUE_PIN; 
            uint8_t GREEN_PIN;

        public: 
            ~RGB_LED();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}