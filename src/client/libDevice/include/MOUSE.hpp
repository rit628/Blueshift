#pragma once

#include "../DeviceCore.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtype/typedefs.hpp"
#include <string>
#include <unordered_map>
#ifdef SDL_ENABLED
#include <SDL3/SDL.h>
#endif



namespace Device {
    class MOUSE : public DeviceCore<TypeDef::MOUSE> {
        private:
        #ifdef SDL_ENABLED
            SDL_MouseID id;
        #endif

        public:
            ~MOUSE();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string>& config);
            void transmitStates(DynamicMessage& dmsg);
            #ifdef SDL_ENABLED
                bool handleMovement(SDL_Event* event);
                bool handleClick(SDL_Event* event);
                bool handleScroll(SDL_Event* event);
            #endif
    };
}