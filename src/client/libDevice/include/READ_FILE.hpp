#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"

template<>
class Device<TypeDef::READ_FILE> : public DeviceCore {
    private: 
        TypeDef::READ_FILE states;
            std::string filename;
            std::ifstream file_stream;

    public: 
        ~Device();
        void processStates(DynamicMessage& dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
};