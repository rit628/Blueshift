#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"
#include "./DHT11_PKG/DHT.hpp"
#include <memory>

namespace Device {
    class DHT11 : public DeviceCore<TypeDef::DHT11> {
        private:
            uint8_t PIN;
            #ifdef __RPI64__
            DHT dht;
            #endif
     
        public:
            ~DHT11();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}