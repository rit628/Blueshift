#pragma once

#include "../../DeviceCore.hpp"
#include "libtypes/typedefs.hpp"
#include <fstream>

/*
    This Interrupt type device reads messages from a file (consdiers the first line)
    and sends the contents of the first line of the file once the return button is pressed
*/
template<>
class Device<TypeDef::LINE_WRITER> : public DeviceCore {
    private:
        TypeDef::LINE_WRITER states;
        std::string filename;
        std::fstream file_stream;
        
    public:
        ~Device();
        void proc_message(DynamicMessage& dmsg);
        bool handleInterrupt();
        void set_ports(std::unordered_map<std::string, std::string> &src);
        void read_data(DynamicMessage &dmsg);
};