#pragma once

#include "DeviceCore.hpp"
#include "typedefs.hpp"
#include <fstream>

namespace Device {
    class TIMER_TEST : public DeviceCore<TypeDef::TIMER_TEST> {
        private: 
        std::string filename;
        std::ofstream write_file; 
        
        public:
        ~TIMER_TEST();
        void processStates(DynamicMessage& dmsg);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
    };
}