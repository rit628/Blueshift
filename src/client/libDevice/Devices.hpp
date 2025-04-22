#pragma once

#include "DeviceCore.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtypes/typedefs.hpp"
#include <atomic>
#include <concepts>
#include <cstdint>
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
            ~TIMER_TEST();

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
            ~LINE_WRITER();

            bool handleInterrupt();
            void set_ports(std::unordered_map<std::string, std::string> &src) override;
            void read_data(DynamicMessage &dmsg) override;
    };

    class LIGHT : public AbstractDevice {
        private:
            TypeDef::LIGHT states;
            uint8_t PIN;

            void proc_message_impl(DynamicMessage& dmsg) override;
        public:
            ~LIGHT();

            void set_ports(std::unordered_map<std::string, std::string> & src) override;
            void read_data(DynamicMessage &dmsg) override;
    };

    class BUTTON : public AbstractDevice
    {
        private: 
            TypeDef::BUTTON states;
            uint8_t PIN;
            std::chrono::high_resolution_clock::time_point lastPress;
            void proc_message_impl(DynamicMessage &dmsg) override;

        public:
            ~BUTTON();
            void set_ports(std::unordered_map<std::string, std::string> &src) override;
            void read_data(DynamicMessage &dmsg) override;
            bool handleInterrupt(int gpio, int level, uint32_t tick);
    };

    class MOTOR : public AbstractDevice 
    {
        private:
            TypeDef::MOTOR states;
            void proc_message_impl(DynamicMessage &dmsg) override;
            int speed;
            int in1_pin, in2_pin, pwm_pin;
            unsigned pwm_range;

        public:
            ~MOTOR();
            void set_ports(std::unordered_map<std::string, std::string> &src) override;
            void read_data(DynamicMessage &dmsg) override;
            void apply_speed(int speed);
    };

    class READ_FILE_POLL : public AbstractDevice{
        private: 
            TypeDef::READ_FILE_POLL states;
            std::string filename;
            std::ifstream file_stream;

            void proc_message_impl(DynamicMessage& dmsg) override;

        public:
            ~READ_FILE_POLL();
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
};

// Device reciever object returns and sets up an object 
std::shared_ptr<AbstractDevice> getDevice(DEVTYPE dtype, std::unordered_map<std::string, std::string> &port_nums, int device_alias);
