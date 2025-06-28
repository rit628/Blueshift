#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

template<>
class Device<TypeDef::LIGHT> : public DeviceCore {
    private:
        TypeDef::LIGHT states;
        uint8_t PIN;

    public:
        ~Device();
        void processStates(DynamicMessage& dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
};
