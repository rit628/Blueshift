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

// Configuration information used in sending types
enum class SrcType{
    UNIX_FILE, 
    GPIO,
    SDL_IO
}; 

struct Interrupt_Desc {
    SrcType src_type;
    std::variant<std::function<bool()>
                , std::function<bool(int, int, uint32_t)>
                #ifdef SDL_ENABLED
                , std::function<bool(SDL_Event*)>
                #endif
                > interruptHandle;
    std::string file_src;
    int port_num;
};

template<typename T>
concept Driveable = TypeDef::DEVTYPE<T>;

template <Driveable T>
class DeviceCore {
    private:
        std::vector<Interrupt_Desc> Idesc_list;
        bool hasInterrupt = false;
    
    protected:
        T states;
    
        // Interrupt watch
        void addFileIWatch(std::string &fileName, std::function<bool()> handler = sendState);
        void addGPIOIWatch(int gpio_port, std::function<bool(int, int, uint32_t)> handler);
        #ifdef SDL_ENABLED
        void addSDLIWatch(std::function<bool(SDL_Event* event)> handler);
        #endif

        friend class DeviceHandle;
};