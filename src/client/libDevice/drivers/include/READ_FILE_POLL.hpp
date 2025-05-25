#pragma once

#include "../../DeviceCore.hpp"
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
        void proc_message(DynamicMessage& dmsg);
        void set_ports(std::unordered_map<std::string, std::string> &src);
        void read_data(DynamicMessage &dmsg);
};