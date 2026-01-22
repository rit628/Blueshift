#pragma once

#include "../DeviceCore.hpp"
#include "typedefs.hpp"

namespace Device {
    class LIGHT : public DeviceCore<TypeDef::LIGHT> {
        private:
            uint8_t PIN;
    
        public:
            ~LIGHT();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}
