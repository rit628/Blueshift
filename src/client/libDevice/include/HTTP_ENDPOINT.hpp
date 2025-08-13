#pragma once
#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"
#include "HttpListener.hpp"
#include <memory>


namespace Device {
    class HTTP_ENDPOINT : public DeviceCore<TypeDef::HTTP_ENDPOINT> {
        private: 
            std::string endpoint;
            std::shared_ptr<HttpListener> listener; 
            
        public: 
            ~HTTP_ENDPOINT();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            bool handleRequest(int sessionID, std::string ip, std::string json); 
    };
}
