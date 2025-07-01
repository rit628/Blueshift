// #pragma once

// #include "../DeviceCore.hpp"
// #include "libDM/DynamicMessage.hpp"
// #include "libtype/typedefs.hpp"
// #include <SDL3/SDL.h>
// #include <string>
// #include <unordered_map>

// namespace Device {
//     class KEYBOARD : public DeviceCore<TypeDef::KEYBOARD> {
//         private:
//             SDL_Window* window = nullptr;
//             SDL_Renderer* renderer = nullptr;

//         public:
//             ~KEYBOARD();
//             void processStates(DynamicMessage& dmsg);
//             void init(std::unordered_map<std::string, std::string>& config);
//             void transmitStates(DynamicMessage& dmsg);
//             bool handleInterrupt();
//     };
// }