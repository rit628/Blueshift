#pragma once

#include "DeviceCore.hpp"
#include "DynamicMessage.hpp"
#include "typedefs.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#ifdef SDL_ENABLED
#include <SDL3/SDL.h>
#endif

namespace Device {
    class GAMEPAD : public DeviceCore<TypeDef::GAMEPAD> {
        private:
            #ifdef SDL_ENABLED
            SDL_JoystickID id;
            SDL_Gamepad* gamepad;
            uint64_t prevSensorTimestamp = 0;
            uint16_t stickDeadzone = 4000;
            double gyroDeadzone = 0.01;
            double pitchRadians = 0, rollRadians = 0;
            const float gyroCoeff = .98;
            #endif

        public:
            ~GAMEPAD();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string>& config);
            void transmitStates(DynamicMessage& dmsg);
            #ifdef SDL_ENABLED
            bool handleButtonPress(SDL_Event* event);
            bool handleAnalogMovement(SDL_Event* event);
            bool handleSensorUpdate(SDL_Event* event);
            #endif
    };
}