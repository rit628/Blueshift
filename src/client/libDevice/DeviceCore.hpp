#pragma once

#include "libDM/DynamicMessage.hpp"
#include "libtype/typedefs.hpp"
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <concepts>
#include <variant>
#ifdef SDL_ENABLED
#include <SDL3/SDL.h>
#endif

inline bool sendState() { return true; }

struct UnixFileInterruptor {
    std::string file;
    std::function<bool()> interruptCallback;
};

struct GpioInterruptor {
    uint8_t portNum;
    std::function<bool(int, int, uint32_t)> interruptCallback;
};

#ifdef SDL_ENABLED
struct SdlIoInterruptor {
    std::function<bool(SDL_Event*)> interruptCallback;
};
#endif

using InterruptDescriptor = std::variant<
      UnixFileInterruptor
    , GpioInterruptor
    #ifdef SDL_ENABLED
    , SdlIoInterruptor
    #endif
>;

template<typename T>
concept Driveable = TypeDef::DEVTYPE<T>;

template <Driveable T>
class DeviceCore {
    private:
        std::vector<InterruptDescriptor> Idesc_list;
    
    protected:
        T states;
    
        // Interrupt watch
        void addFileIWatch(std::string &fileName, std::function<bool()> handler = sendState);
        void addGPIOIWatch(uint8_t gpioPort, std::function<bool(int, int, uint32_t)> handler);
        #ifdef SDL_ENABLED
        void addSDLIWatch(std::function<bool(SDL_Event* event)> handler);
        #endif

        friend class DeviceHandle;
};