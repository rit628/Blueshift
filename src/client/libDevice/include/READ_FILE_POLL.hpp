#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"
#include <fstream>

template<>
class Device<TypeDef::READ_FILE_POLL> : public DeviceCore {
    private: 
        TypeDef::READ_FILE_POLL states;
        std::string filename;
        std::ifstream file_stream;
        
    public:
        ~Device();
        void processStates(DynamicMessage& dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
};