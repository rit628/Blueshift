#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

namespace Device {
    class BUTTON : public DeviceCore<TypeDef::BUTTON> {
        private:
            uint8_t PIN;
            std::chrono::high_resolution_clock::time_point lastPress;
            
        public:
            ~BUTTON();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            bool handleInterrupt(int gpio, int level, uint32_t tick);
    };
}