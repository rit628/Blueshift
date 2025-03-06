#pragma once
#include "bls_types.hpp"
#include "include/Common.hpp"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/regex.hpp>

namespace BlsLang {

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