#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"
#include <fstream>

template<>
class Device<TypeDef::TIMER_TEST> : public DeviceCore {
    private: 
        TypeDef::TIMER_TEST states;
        std::string filename;
        std::ofstream write_file; 
        
    public:
        ~Device();
        void processStates(DynamicMessage& dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
};