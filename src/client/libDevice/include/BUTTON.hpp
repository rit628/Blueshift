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
        void proc_message(DynamicMessage &dmsg);
        void set_ports(std::unordered_map<std::string, std::string> &src);
        void read_data(DynamicMessage &dmsg);
        bool handleInterrupt(int gpio, int level, uint32_t tick);
};