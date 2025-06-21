#pragma once

#include "../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"

namespace Device {
    class FILE_LOG : public DeviceCore<TypeDef::FILE_LOG> {
        private: 
            std::string filename;
            std::ofstream outStream;
    
        public: 
            ~FILE_LOG();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}