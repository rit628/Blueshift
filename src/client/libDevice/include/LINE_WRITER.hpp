#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"
#include <fstream>

/*
    This Interrupt type device reads messages from a file (consdiers the first line)
    and sends the contents of the first line of the file once the return button is pressed
*/
namespace Device {
    class LINE_WRITER : public DeviceCore<TypeDef::LINE_WRITER> {
        private:
            std::string filename;
            std::fstream file_stream;
            
        public:
            ~LINE_WRITER();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            bool handleInterrupt();
    };
}