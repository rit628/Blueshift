#pragma once
#include "bls_types.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <iostream>

namespace BlsLang {

    struct DeviceDescriptor{
        std::string device_name; 
        DEVTYPE devtype; 
    
        std::string controller; 
        std::vector<uint8_t> port_maps; 
    
        bool isInterrupt; 
    
        // May not need to be used: 
        bool isVtype; 
    }; 
    
    struct OBlockDesc{
    
        /*
            Normal State
        */
    
        std::string name; 
        std::vector<DeviceDescriptor> binded_devices; 
        int bytecode_offset; 
    
        // Reading Config
    
        /*
            dropRead :if true -> only read all recieving states once Oblock execution is finished, drop all others
            dropWrite: if true -> Only write to mailbox with the callback is open: 
        */
    
        bool dropRead; 
        bool dropWrite; 
    
        // Configuration (all time in milliseconds)
    
        /*
            Max pollrate: corresponds to the polling rate of all polling devices binded to the oblock
            Const Poll: Is the polling rate constant (true until ticker table implementation)
            Synchronize State: Block until all states of refreshed (true for now)
        
        */
        int max_pollrate = 500; 
        bool const_poll = true; 
        bool synchronize_states = true; 
    };

    inline DeviceDescriptor parseDeviceBinding(const std::string deviceName, DEVTYPE devtype, const std::string& binding) {
        static boost::regex bindingPattern(R"(([a-zA-Z0-9_\-]+)::[a-zA-Z]+-(\d+)(?:, ?[a-zA-Z]+-(\d+))*)");
        boost::smatch bindingContents;
        if (!boost::regex_match(binding, bindingContents, bindingPattern)) {
            throw std::runtime_error("Invalid binding string");
        }
        DeviceDescriptor result;
        result.device_name = deviceName;
        result.devtype = devtype;
        result.controller = bindingContents[1];
        for (int i = 2; i < bindingContents.size(); i++) {
            std::string port = bindingContents[i].str();
            if (!port.empty()) {
                result.port_maps.push_back(std::stoi(port));
            }
        }
        return result;
    }

}