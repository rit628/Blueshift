#pragma once

#include "../../DeviceCore.hpp"
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
        void proc_message(DynamicMessage& dmsg);
        void set_ports(std::unordered_map<std::string, std::string> &srcs);
        void read_data(DynamicMessage &dmsg);
};