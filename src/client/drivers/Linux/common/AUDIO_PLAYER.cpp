#if defined(__linux__) &&  defined(SDL_ENABLED)

#include "AUDIO_PLAYER.hpp"
#include "Protocol.hpp"
#include "Connection.hpp"
#include "DynamicMessage.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <SDL3/SDL.h>

using namespace Device;

void AUDIO_PLAYER::processStates(DynamicMessage& dmsg) {
    auto currentFile = states.file;
    auto currentVolume = states.volume;
    dmsg.unpackStates(states);
    
    if (states.file != "" && states.file != currentFile) {
        auto newFile = directory / states.file;
        std::cout << newFile << std::endl;
        if (!SDL_LoadWAV(newFile.c_str(), &spec, &audioData, &audioLength)) {
            SDL_Log("Invalid file provided: %s", SDL_GetError());
        }
        else {
            SDL_ClearAudioStream(stream);
            SDL_SetAudioStreamFormat(stream, &spec, nullptr);
            SDL_PutAudioStreamData(stream, audioData, audioLength);
        }
    }
    if (states.volume != currentVolume) {
        states.volume = std::max(int64_t(0), std::min(states.volume, int64_t(100)));
        std::cout << "VOLUME: " << states.volume << std::endl;
        float newVolume =  states.volume / 100.0;
        std::cout << "NEW VOLUME: " << newVolume << std::endl;
        SDL_SetAudioStreamGain(stream, newVolume);
    }
    if (states.paused) {
        SDL_PauseAudioStreamDevice(stream);
    }
    else {
        if (SDL_GetAudioStreamAvailable(stream) == 0) {
            SDL_PutAudioStreamData(stream, audioData, audioLength);
        }
        SDL_ResumeAudioStreamDevice(stream);
    }
}

void AUDIO_PLAYER::init(std::unordered_map<std::string, std::string>& config) {
    directory = config.at("directory");
    if (!std::filesystem::exists(directory)) {
        throw BlsExceptionClass("Invalid directory supplied.", ERROR_T::BAD_DEV_CONFIG);
    }
    
    auto createStream = [](void* self) -> void {
        auto& audioPlayer = *reinterpret_cast<AUDIO_PLAYER*>(self);

        if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            throw BlsExceptionClass("Failed to initialize SDL audio subsystem: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
        }

        audioPlayer.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr, nullptr, nullptr);
    };

    if (!SDL_RunOnMainThread(createStream, this, true) || !stream) {
        throw BlsExceptionClass("Failed to create audio stream: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
    }

    SDL_SetHint("SDL_HINT_INPUT_WINDOW_REQUIRED", "true");
}

void AUDIO_PLAYER::transmitStates(DynamicMessage& dmsg) {
    states.paused = SDL_AudioStreamDevicePaused(stream);
    states.volume = SDL_GetAudioStreamGain(stream) * 100;
    dmsg.packStates(states);
}

AUDIO_PLAYER::~AUDIO_PLAYER() {
    SDL_DestroyAudioStream(stream);
}

#endif