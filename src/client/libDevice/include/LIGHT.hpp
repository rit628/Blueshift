#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"

template<>
class Device<TypeDef::LIGHT> : public DeviceCore {
    private:
        TypeDef::LIGHT states;
        uint8_t PIN;

    public:
        ~Device();
        void proc_message(DynamicMessage& dmsg);
        void set_ports(std::unordered_map<std::string, std::string> & src);
        void read_data(DynamicMessage &dmsg);
};
