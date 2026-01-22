#pragma once
#include "../DeviceCore.hpp"
#include "typedefs.hpp"


// THE DC MOTOR IS ASSUMED TO BE IMPLEMENTED USING THE L293D MOTOR DRIVER
namespace Device{
    class DC_MOTOR : public DeviceCore<TypeDef::DC_MOTOR>{
        private: 
            uint8_t enablePin;
            uint8_t cw_pin1;
            uint8_t ccw_pin2;

        public: 
            ~DC_MOTOR();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}