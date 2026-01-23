#if defined(__linux__) &&  defined(SDL_ENABLED)

#include "MOUSE.hpp"
#include "Protocol.hpp"
#include "Connection.hpp"
#include "DynamicMessage.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <SDL3/SDL.h>

using namespace Device;

void MOUSE::processStates(DynamicMessage&) {
    // read only
}

void MOUSE::init(std::unordered_map<std::string, std::string>& config) {
    auto& mode = config.at("mode");
    if (mode == "all") {
        id = 0;
    }
    else if (mode == "select") {
        auto selectMouse = [](void* self) -> void {
            auto& mouse = *reinterpret_cast<MOUSE*>(self);
            auto window = SDL_GL_GetCurrentWindow();
            showAndFocusWindow(window);
            SDL_Log("Waggle the mouse you would like to use...\n");
            while (true) {
                SDL_Event event;
                SDL_WaitEvent(&event);
                if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    mouse.id = event.motion.which;
                    SDL_Log("Motion detected on mouse \"%s\" with id: %d\n", SDL_GetMouseNameForID(mouse.id), mouse.id);
                    break;
                }
            }
            SDL_HideWindow(window);
        };
        if (!SDL_RunOnMainThread(selectMouse, this, true)) {
            throw BlsExceptionClass("Failed to select mouse: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
        }
    }
    else {
        throw BlsExceptionClass("Invalid mouse input mode.", ERROR_T::BAD_DEV_CONFIG);
    }
    SDL_SetHint("SDL_HINT_INPUT_WINDOW_REQUIRED", "true");
    addSDLIWatch(std::bind(&MOUSE::handleMovement, std::ref(*this), std::placeholders::_1));
    addSDLIWatch(std::bind(&MOUSE::handleClick, std::ref(*this), std::placeholders::_1));
    addSDLIWatch(std::bind(&MOUSE::handleScroll, std::ref(*this), std::placeholders::_1));
}

void MOUSE::transmitStates(DynamicMessage& dmsg) {
    dmsg.packStates(states);
}

bool MOUSE::handleMovement(SDL_Event* event) {
    if (event->type == SDL_EVENT_MOUSE_MOTION && (id == 0 || event->motion.which == id)) {
        states.x = event->motion.x;
        states.y = event->motion.y;
        return true;
    }
    return false;
}

bool MOUSE::handleClick(SDL_Event* event) {
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && (id == 0 || event->button.which == id)) {
        switch (event->button.button) {
            case SDL_BUTTON_LEFT:
                states.leftClick = true;
            break;
            case SDL_BUTTON_RIGHT:
                states.rightClick = true;
            break;
            case SDL_BUTTON_MIDDLE:
                states.middleClick = true;
            break;
        }
        return true;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && (id == 0 || event->button.which == id)) {
        switch (event->button.button) {
            case SDL_BUTTON_LEFT:
                states.leftClick = false;
            break;
            case SDL_BUTTON_RIGHT:
                states.rightClick = false;
            break;
            case SDL_BUTTON_MIDDLE:
                states.middleClick = false;
            break;
        }
        return true;
    }
    return false;
}

bool MOUSE::handleScroll(SDL_Event* event) {
    if (event->type == SDL_EVENT_MOUSE_WHEEL && (id == 0 || event->wheel.which == id)) {
        states.scrollX = event->wheel.integer_x;
        states.scrollY = event->wheel.integer_y;
        return true;
    }
    else {
        states.scrollX = 0;
        states.scrollY = 0;
    }
    return false;
}

#endif