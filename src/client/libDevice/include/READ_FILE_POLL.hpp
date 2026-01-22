#pragma once

#include "../DeviceCore.hpp"
#include "typedefs.hpp"
#include <fstream>

namespace Device {
    class READ_FILE_POLL : public DeviceCore<TypeDef::READ_FILE_POLL> {
        private: 
            std::string filename;
            std::ifstream file_stream;
            
        public:
            ~READ_FILE_POLL();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}