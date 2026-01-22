#pragma once

#include "../DeviceCore.hpp"
#include "DynamicMessage.hpp"
#include "typedefs.hpp"
#include <string>
#include <unordered_map>
#ifdef SDL_ENABLED
#include <SDL3/SDL.h>
#endif

namespace Device {
    class KEYBOARD : public DeviceCore<TypeDef::KEYBOARD> {
        private:
            #ifdef SDL_ENABLED
            SDL_KeyboardID id;
            #endif

        public:
            ~KEYBOARD();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string>& config);
            void transmitStates(DynamicMessage& dmsg);
            #ifdef SDL_ENABLED
            bool handleInterrupt(SDL_Event* event);
            #endif
    };
}