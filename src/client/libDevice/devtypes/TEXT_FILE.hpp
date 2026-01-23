#pragma once

#include "DeviceCore.hpp"
#include "TSQ.hpp"
#include "typedefs.hpp"
#include <shared_mutex>

namespace Device {
    class TEXT_FILE : public DeviceCore<TypeDef::TEXT_FILE> {
        private:
            std::string filename;
            std::shared_mutex m;

        public:
            ~TEXT_FILE();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}