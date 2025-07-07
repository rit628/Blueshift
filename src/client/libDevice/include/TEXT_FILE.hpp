#pragma once

#include "../DeviceCore.hpp"
#include "libTSQ/TSQ.hpp"
#include "libtype/typedefs.hpp"
#include <shared_mutex>

namespace Device {
    class TEXT_FILE : public DeviceCore<TypeDef::TEXT_FILE> {
        private:
            std::string filename;
            TSQ<TypeDef::TEXT_FILE> queryResults;
            std::shared_mutex m;

        public:
            ~TEXT_FILE();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}