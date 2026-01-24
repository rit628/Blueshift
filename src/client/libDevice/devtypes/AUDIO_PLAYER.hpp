#pragma once

#include "DeviceCore.hpp"
#include "typedefs.hpp"
#include <cstdint>
#include <filesystem>

namespace Device {
    class AUDIO_PLAYER : public DeviceCore<TypeDef::AUDIO_PLAYER> {
        private:
            std::filesystem::path directory;
            // dont know why these are registering as unused
            uint8_t* audioData [[ maybe_unused ]];
            uint32_t audioLength [[ maybe_unused ]];
            #ifdef SDL_ENABLED
            SDL_AudioStream* stream [[ maybe_unused ]] = nullptr;
            SDL_AudioSpec spec [[ maybe_unused ]];
            #endif

        public:
            ~AUDIO_PLAYER();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}