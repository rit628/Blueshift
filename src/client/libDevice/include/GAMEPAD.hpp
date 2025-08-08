#pragma once

#include "../DeviceCore.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtype/typedefs.hpp"
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
        #endif

        public:
            ~GAMEPAD();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string>& config);
            void transmitStates(DynamicMessage& dmsg);
            #ifdef SDL_ENABLED
            bool handleButtonPress(SDL_Event* event);
            bool handleAnalogMovement(SDL_Event* event);
            #endif
    };
}