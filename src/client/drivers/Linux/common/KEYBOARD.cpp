// #ifdef __linux__

// #include "include/KEYBOARD.hpp"
// #include "libDM/DynamicMessage.hpp"
// #include <string>
// #include <unordered_map>

// using namespace Device;

// void KEYBOARD::processStates(DynamicMessage& dmsg) {
//     // read only
//     dmsg.unpackStates(states);
// }

// void KEYBOARD::init(std::unordered_map<std::string, std::string>& config) {
//     if (!SDL_CreateWindowAndRenderer("Keyboard Log", 1280, 720, SDL_WINDOW_FULLSCREEN, &this->window, &this->renderer)) {
//         std:: cerr << "Keyboard Log window creation failed: " << SDL_GetError();
//         return;
//     }
// }

// void KEYBOARD::transmitStates(DynamicMessage& dmsg) {
    
// }

// #endif