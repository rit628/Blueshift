#if defined(__linux__) &&  defined(SDL_ENABLED)

#include "include/KEYBOARD.hpp"
#include "libnetwork/Protocol.hpp"
#include "libnetwork/Connection.hpp"
#include "libDM/DynamicMessage.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <SDL3/SDL.h>

using namespace Device;

void KEYBOARD::processStates(DynamicMessage& dmsg) {
    // read only
}

void KEYBOARD::init(std::unordered_map<std::string, std::string>& config) {
    auto& mode = config.at("mode");
    if (mode == "all") {
        id = 0;
    }
    else if (mode == "select") {
        auto selectKeyboard = [](void* self) -> void {
            auto& keyboard = *reinterpret_cast<KEYBOARD*>(self);
            auto window = SDL_GL_GetCurrentWindow();
            SDL_ShowWindow(window);
            SDL_Log("Press a key on the keyboard you would like to use...\n");
            while (true) {
                SDL_Event event;
                SDL_WaitEvent(&event);
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    keyboard.id = event.kdevice.which;
                    SDL_Log("Keypress '%s' detected on keyboard \"%s\" with id: %d\n", SDL_GetKeyName(event.key.key), SDL_GetKeyboardNameForID(keyboard.id), keyboard.id);
                    break;
                }
            }
            SDL_HideWindow(window);
        };
        if (!SDL_RunOnMainThread(selectKeyboard, this, true)) {
            throw BlsExceptionClass("Failed to select keyboard: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
        }
    }
    else {
        throw BlsExceptionClass("Invalid keyboard input mode.", ERROR_T::BAD_DEV_CONFIG);
    }
    SDL_SetHint("SDL_HINT_INPUT_WINDOW_REQUIRED", "true");
    addSDLIWatch(std::bind(&KEYBOARD::handleInterrupt, std::ref(*this), std::placeholders::_1));
}

void KEYBOARD::transmitStates(DynamicMessage& dmsg) {
    dmsg.packStates(states);
}

bool KEYBOARD::handleInterrupt(SDL_Event* event) {
    if (event->type == SDL_EVENT_KEY_DOWN && (id == 0 || event->kdevice.which == id)) {
        states.key = SDL_GetKeyName(event->key.key);
        return true;
    }
    return false;
}

#endif