#pragma once

#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"
#include <cstdint>
#include <filesystem>

namespace Device {
    class AUDIO_PLAYER : public DeviceCore<TypeDef::AUDIO_PLAYER> {
        private:
            std::filesystem::path directory;
            uint8_t* audioData;
            uint32_t audioLength;
            #ifdef SDL_ENABLED
                SDL_AudioStream* stream = nullptr;
                SDL_AudioSpec spec;
            #endif

        public:
            ~AUDIO_PLAYER();
            void processStates(DynamicMessage& dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
    };
}