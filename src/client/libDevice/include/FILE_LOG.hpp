#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"

template<>
class Device<TypeDef::FILE_LOG> : public DeviceCore {
    private: 
        TypeDef::FILE_LOG states; 
        std::string filename;
        std::ofstream outStream;

    public: 
        ~Device();
        void processStates(DynamicMessage& dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
};