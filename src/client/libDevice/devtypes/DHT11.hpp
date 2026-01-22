#pragma once

#include "DeviceCore.hpp"
#include "typedefs.hpp"
#include "include/DHT11/DHT.hpp"

namespace Device {
    class DHT11 : public DeviceCore<TypeDef::DHT11> {
        private:
            uint8_t PIN;
            DHT dht;
     
        public:
            ~DHT11();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}