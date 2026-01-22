#pragma once
#include "../DeviceCore.hpp"
#include "typedefs.hpp"


// THE DC MOTOR IS ASSUMED TO BE IMPLEMENTED USING THE L293D MOTOR DRIVER
namespace Device{
    class STEP_MOTOR : public DeviceCore<TypeDef::STEP_MOTOR>{
        private: 
            std::vector<uint8_t> portList;
            const int CCWStep[4] {0x01, 0x02, 0x04, 0x08}; 
            const int CWStep[4] {0x08, 0x04, 0x02, 0x01};
            
            // Delay is set to minimum delay (max speed) of 2
            const int ms_delay = 2;

        public: 
            ~STEP_MOTOR();
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            void moveOneStep(bool cw);
            void moveNSteps(bool cw, int steps);
    };
}