#pragma once
#include "bls_types.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/regex.hpp>

namespace BlsLang {

    struct DeviceDescriptor{
        std::string device_name; 
        DEVTYPE devtype; 
    
        std::string controller; 
        std::unordered_map<std::string, std::string> port_maps; 
    
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
        static boost::regex bindingPattern(R"(([a-zA-Z0-9_\-]+)::([a-zA-Z]+-[^,\- ]+(?:,[a-zA-Z]+-[^,\- ]+)*))");
        boost::smatch bindingContents;
        if (!boost::regex_match(binding, bindingContents, bindingPattern)) {
            throw std::runtime_error("Invalid binding string");
        }
        DeviceDescriptor result;
        result.device_name = deviceName;
        result.devtype = devtype;
        result.controller = bindingContents[1];
        std::string portMap = bindingContents[2];
        int idx = 0;

        while (idx < portMap.size()) {
            int delimIdx = portMap.find_first_of('-', idx);
            std::string role = portMap.substr(idx, delimIdx - idx);
            idx = delimIdx + 1;
            delimIdx = portMap.find_first_of(',', idx);
            if (delimIdx == std::string::npos) {
                delimIdx = portMap.size();
            }
            std::string port = portMap.substr(idx, delimIdx - idx);
            result.port_maps.emplace(std::move(role), std::move(port));
            idx = delimIdx + 1;
        }
        return result;
    }

}