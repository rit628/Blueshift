#if defined(__linux__) &&  defined(SDL_ENABLED)

#include "include/GAMEPAD.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libnetwork/Connection.hpp"
#include <fstream>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <SDL3/SDL.h>

using namespace Device;
using namespace std::string_literals;

void GAMEPAD::processStates(DynamicMessage& dmsg) {
    // rumble attributes are the only writeable states
    dmsg.unpack("leftRumble", states.leftRumble);
    dmsg.unpack("rightRumble", states.rightRumble);
    dmsg.unpack("rumbleDuration", states.rumbleDuration);

    static auto clampIntensity = [](int64_t intensity) -> uint16_t {
        return std::max((int64_t)0, std::min(intensity, (int64_t)UINT16_MAX));
    };
    static auto clampDuration = [](int64_t duration) -> uint32_t {
        return std::max((int64_t)0, std::min(duration, (int64_t)UINT32_MAX));
    };

    SDL_RumbleGamepad(gamepad
                    , clampIntensity(states.leftRumble)
                    , clampIntensity(states.rightRumble)
                    , clampDuration(states.rumbleDuration));
}

void GAMEPAD::init(std::unordered_map<std::string, std::string>& config) {   
    auto selectGamepad = [](void* self) -> void {
        auto& gamepad = *reinterpret_cast<GAMEPAD*>(self);

        if (!SDL_WasInit(SDL_INIT_GAMEPAD) && !SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            throw BlsExceptionClass("Failed to initialize SDL gamepad subsystem: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
        }
        
        auto addGamepad = [](SDL_JoystickID id) {
            auto* connected = SDL_OpenGamepad(id);
            SDL_Log("Gamepad connected: %s\n", SDL_GetGamepadName(connected));
        };

        int count = 0;
        auto* gamepadIds = SDL_GetGamepads(&count);
        for (int i = 0; i < count; i++) {
            addGamepad(gamepadIds[i]);
        }
        
        auto window = SDL_GL_GetCurrentWindow();
        showAndFocusWindow(window);
        SDL_Log("Press a button on the gamepad you would like to use...\n");
        while (true) {
            SDL_Event event;
            SDL_WaitEvent(&event);
            if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
                addGamepad(event.gdevice.which);
            }
            if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN || event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
                gamepad.id = event.gdevice.which;
                gamepad.gamepad = SDL_GetGamepadFromID(gamepad.id);
                SDL_Log("Gamepad %s selected!", SDL_GetGamepadName(gamepad.gamepad));
                break;
            }
        }
        SDL_HideWindow(window);
        SDL_SetHint("SDL_HINT_INPUT_WINDOW_REQUIRED", "true");
    };

    if (!SDL_RunOnMainThread(selectGamepad, this, true)) {
        throw BlsExceptionClass("Failed to select gamepad: " + std::string(SDL_GetError()), ERROR_T::BAD_DEV_CONFIG);
    }

    auto& mapping = config.at("mapping");

    if (mapping == "automatic") {
        SDL_Log("Button mappings automatically configured.");
    }
    else if (mapping == "manual") {
        auto mapButtons = [](void* self) -> void {
            auto& gamepad = *reinterpret_cast<GAMEPAD*>(self);
            char guidBuff[33];
            auto guid = SDL_GetGamepadGUIDForID(gamepad.id);
            SDL_GUIDToString(guid, guidBuff, 33);
            std::string mapping = guidBuff + ","s + SDL_GetGamepadName(gamepad.gamepad) + ","s;

            auto window = SDL_GL_GetCurrentWindow();
            showAndFocusWindow(window);

            auto mapButtonInput = [&gamepad, &mapping](std::string&& buttonName, SDL_GamepadButton button) {
                SDL_Log("Press the %s button.\n", buttonName.c_str());
                auto buttonString = SDL_GetGamepadStringForButton(button);
                while (true) {
                    SDL_Event event;
                    SDL_WaitEvent(&event);
                    if (event.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN && event.jbutton.which == gamepad.id) {
                        auto buttonVal = "b"s + std::to_string(event.jbutton.button);
                        mapping += buttonString + ":"s + buttonVal + ","s;
                        break;
                    }
                    else if (event.type == SDL_EVENT_JOYSTICK_HAT_MOTION && event.jhat.which == gamepad.id) {
                        if (event.jhat.value == 0) { // skip centered dpad
                            continue;
                        }
                        auto buttonVal = "h"s + std::to_string(event.jhat.hat) + "." + std::to_string(event.jhat.value);
                        mapping += buttonString + ":"s + buttonVal + ","s;
                        break;
                    }
                }
            };
            auto mapAnalogInput = [&gamepad, &mapping](std::string&& inputName, SDL_GamepadAxis input) {
                SDL_Log("Hold the %s then press any button.\n", inputName.c_str());
                uint8_t inputVal = 0;
                while (true) {
                    SDL_Event event;
                    SDL_WaitEvent(&event);
                    if (event.type == SDL_EVENT_JOYSTICK_AXIS_MOTION && event.jaxis.which == gamepad.id) {
                        inputVal = event.jaxis.axis;
                    }
                    else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event.gaxis.which == gamepad.id) {
                        auto axisVal = "a"s + std::to_string(inputVal);
                        mapping += SDL_GetGamepadStringForAxis(input) + ":"s + axisVal + ","s;
                        break;
                    }
                }
            };

            mapButtonInput("a", SDL_GAMEPAD_BUTTON_SOUTH);
            mapButtonInput("b", SDL_GAMEPAD_BUTTON_EAST);
            mapButtonInput("x", SDL_GAMEPAD_BUTTON_WEST);
            mapButtonInput("y", SDL_GAMEPAD_BUTTON_NORTH);

            mapButtonInput("dpadUp", SDL_GAMEPAD_BUTTON_DPAD_UP);
            mapButtonInput("dpadDown", SDL_GAMEPAD_BUTTON_DPAD_DOWN);
            mapButtonInput("dpadLeft", SDL_GAMEPAD_BUTTON_DPAD_LEFT);
            mapButtonInput("dpadRight", SDL_GAMEPAD_BUTTON_DPAD_RIGHT);

            mapButtonInput("lsb", SDL_GAMEPAD_BUTTON_LEFT_STICK);
            mapButtonInput("rsb", SDL_GAMEPAD_BUTTON_RIGHT_STICK);

            mapButtonInput("lb", SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
            mapButtonInput("rb", SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);

            mapButtonInput("start", SDL_GAMEPAD_BUTTON_START);
            mapButtonInput("select", SDL_GAMEPAD_BUTTON_BACK);
            mapButtonInput("home", SDL_GAMEPAD_BUTTON_GUIDE);

            mapAnalogInput("left trigger", SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
            mapAnalogInput("right trigger", SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);

            mapAnalogInput("left stick horizontally", SDL_GAMEPAD_AXIS_LEFTX);
            mapAnalogInput("left stick vertically", SDL_GAMEPAD_AXIS_LEFTY);
            mapAnalogInput("right stick horizontally", SDL_GAMEPAD_AXIS_RIGHTX);
            mapAnalogInput("right stick vertically", SDL_GAMEPAD_AXIS_RIGHTY);

            mapping += "platform:"s + std::string(SDL_GetPlatform()) + ","s;

            if (SDL_AddGamepadMapping(mapping.c_str()) != -1) { // write successful mapping to file for reuse
                std::ofstream out("./samples/client/"s + guidBuff + ".txt"s);
                out.write(mapping.c_str(), mapping.size());
                out.close();
            }
        };

        if (!SDL_RunOnMainThread(mapButtons, this, true)) {
            throw BlsExceptionClass("Failed to complete gamepad mapping: " + std::string(SDL_GetError()), ERROR_T::BAD_DEV_CONFIG);
        }
    }
    else {
        if (SDL_AddGamepadMappingsFromFile(("./samples/client/"s + mapping).c_str()) < 0) {
            throw BlsExceptionClass("Invalid gamepad mapping file supplied: " + std::string(SDL_GetError()), ERROR_T::BAD_DEV_CONFIG);
        }
    }
    addSDLIWatch(std::bind(&GAMEPAD::handleButtonPress, std::ref(*this), std::placeholders::_1));
    addSDLIWatch(std::bind(&GAMEPAD::handleAnalogMovement, std::ref(*this), std::placeholders::_1));
}

void GAMEPAD::transmitStates(DynamicMessage& dmsg) {
    dmsg.packStates(states);
}

bool GAMEPAD::handleButtonPress(SDL_Event* event) {
    if (event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && event->gbutton.which == id) {
        bool pressed = true;
        switch (event->gbutton.button) {
            case SDL_GAMEPAD_BUTTON_SOUTH: 
                states.a = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_EAST: 
                states.b = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_WEST: 
                states.x = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_NORTH: 
                states.y = pressed;
            break;
    
            case SDL_GAMEPAD_BUTTON_DPAD_UP: 
                states.dpadUp = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: 
                states.dpadDown = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: 
                states.dpadLeft = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: 
                states.dpadRight = pressed;
            break;
    
            case SDL_GAMEPAD_BUTTON_LEFT_STICK: 
                states.lsb = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK: 
                states.rsb = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: 
                states.lb = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: 
                states.rb = pressed;
            break;
            
    
            case SDL_GAMEPAD_BUTTON_START: 
                states.start = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_BACK: 
                states.select = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_GUIDE: 
                states.home = pressed;
            break;
        }
        return true;
    }
    else if (event->type == SDL_EVENT_GAMEPAD_BUTTON_UP && event->gbutton.which == id) {
        bool pressed = false;
        switch (event->gbutton.button) {
            case SDL_GAMEPAD_BUTTON_SOUTH: 
                states.a = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_EAST: 
                states.b = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_WEST: 
                states.x = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_NORTH: 
                states.y = pressed;
            break;
    
            case SDL_GAMEPAD_BUTTON_DPAD_UP: 
                states.dpadUp = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: 
                states.dpadDown = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: 
                states.dpadLeft = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: 
                states.dpadRight = pressed;
            break;
    
            case SDL_GAMEPAD_BUTTON_LEFT_STICK: 
                states.lsb = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK: 
                states.rsb = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: 
                states.lb = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: 
                states.rb = pressed;
            break;
            
    
            case SDL_GAMEPAD_BUTTON_START: 
                states.start = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_BACK: 
                states.select = pressed;
            break;
            case SDL_GAMEPAD_BUTTON_GUIDE: 
                states.home = pressed;
            break;
        }
        return true;
    }
    return false;
}

bool GAMEPAD::handleAnalogMovement(SDL_Event* event) {
    if (event->type == SDL_EVENT_GAMEPAD_AXIS_MOTION && event->gaxis.which == id) {
        auto value = event->gaxis.value;
        switch (event->gaxis.axis) {
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                states.lt = value;
            break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                states.rt = value;
            break;
            
            case SDL_GAMEPAD_AXIS_LEFTX:
                states.leftStickX = value;
            break;
            case SDL_GAMEPAD_AXIS_LEFTY:
                states.leftStickY = value;
            break;
            case SDL_GAMEPAD_AXIS_RIGHTX:
                states.rightStickX = value;
            break;
            case SDL_GAMEPAD_AXIS_RIGHTY:
                states.rightStickY = value;
            break;
        }
        return true;
    }
    return false;
}

GAMEPAD::~GAMEPAD() {
    SDL_CloseGamepad(gamepad);
}

#endif