#pragma once
#include "error_types.hpp"
#include "include/Common.hpp"
#include <string>
#include <unordered_map>
#include <boost/regex.hpp>

namespace BlsLang {

    inline DeviceDescriptor parseDeviceBinding(const std::string deviceName, TYPE type, const std::string& binding) {
        static boost::regex bindingPattern(R"(([a-zA-Z0-9_\-]+)::([a-zA-Z_]+-[^,\- ]+(?:,[a-zA-Z_]+-[^,\- ]+)*))");
        boost::smatch bindingContents;
        if (!boost::regex_match(binding, bindingContents, bindingPattern)) {
            throw RuntimeError("Invalid binding string");
        }
        DeviceDescriptor result;
        result.device_name = deviceName;
        result.type = type;
        result.controller = bindingContents[1];
        // Make polling rates dynamic
        result.isConst = true; 
        result.isInterrupt = false; 
        result.isVtype = false; 
        result.polling_period = 1000; 
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