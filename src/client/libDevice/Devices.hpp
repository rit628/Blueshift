#pragma once

#include "DeviceCore.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtypes/bls_types.hpp"
#include "libtypes/typedefs.hpp"
#include <atomic>
#include <concepts>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sys/fcntl.h>
#include <chrono>
#include <thread>

namespace Device {
    #define DEVTYPE_BEGIN(name) \
    class name;
    #define ATTRIBUTE(...)
    #define DEVTYPE_END
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
    
    class TIMER_TEST : public AbstractDevice {
        private: 
            TypeDef::TIMER_TEST states;
            std::string filename;
            std::ofstream write_file; 

            void proc_message_impl(DynamicMessage& dmsg) override;
        
        public:
            void set_ports(std::unordered_map<std::string, std::string> &srcs) override;
            void read_data(DynamicMessage &dmsg) override;
        
    };

    /*
        This Interrupt type device reads messages from a file (consdiers the first line)
        and sends the contents of the first line of the file once the return button is pressed
    */
    class LINE_WRITER : public AbstractDevice {
        private:
            TypeDef::LINE_WRITER states;
            std::string filename;
            std::fstream file_stream;
            

            void proc_message_impl(DynamicMessage& dmsg) override;

        public:
            bool handleInterrupt();
            void set_ports(std::unordered_map<std::string, std::string> &src) override;
            void read_data(DynamicMessage &dmsg) override;
            ~LINE_WRITER(); 
    };

    class READ_FILE : public AbstractDevice{
        private: 
            TypeDef::READ_FILE states;
            std::string filename;
            std::ifstream file_stream;

            void proc_message_impl(DynamicMessage& dmsg) override;
            bool handleInterrupt(); 

        public:
            ~READ_FILE();
            void set_ports(std::unordered_map<std::string, std::string> &src) override;
            void read_data(DynamicMessage &dmsg) override;
            

    }; 

    class FILE_LOG : public AbstractDevice{
        private: 
            TypeDef::FILE_LOG states; 
            std::string filename;
            std::ofstream outStream; 
            void proc_message_impl(DynamicMessage &dmsg) override; 

        public: 
            ~FILE_LOG(); 
            void set_ports(std::unordered_map<std::string, std::string> &src) override; 
            void read_data(DynamicMessage &dmsg) override; 
    }; 

    #define DEVTYPE_BEGIN(name) \
    static_assert(std::derived_from<name, AbstractDevice>, #name " must inherit from AbstractDevice");
    #define ATTRIBUTE(...)
    #define DEVTYPE_END
    #include "DEVTYPES.LIST"
    #undef DEVTYPE_BEGIN
    #undef ATTRIBUTE
    #undef DEVTYPE_END
}

// Device reciever object returns and sets up an object 
std::shared_ptr<AbstractDevice> getDevice(DEVTYPE dtype, std::unordered_map<std::string, std::string> &port_nums, int device_alias);
