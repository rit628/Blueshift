#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

namespace Device {
    class READ_FILE : public DeviceCore<TypeDef::READ_FILE> {
        private:
            std::string filename;
            std::ifstream file_stream;
        
        public: 
            ~READ_FILE();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}