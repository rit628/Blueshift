#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"

template<>
class Device<TypeDef::BUTTON> : public DeviceCore {
    private:
        TypeDef::BUTTON states;
        uint8_t PIN;
        std::chrono::high_resolution_clock::time_point lastPress;
        
    public:
        ~Device();
        void processStates(DynamicMessage &dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
        bool handleInterrupt(int gpio, int level, uint32_t tick);
};