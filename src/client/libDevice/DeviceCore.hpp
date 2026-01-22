#pragma once

#include "DynamicMessage.hpp"
#include "include/HttpListener.hpp"
#include "TSQ.hpp"
#include "typedefs.hpp"
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <concepts>
#include <utility>
#include <variant>
#include "include/ADC.hpp"
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

struct HttpInterruptor{
    std::shared_ptr<HttpListener> server; 
    std::string endpoint; 
    std::function<bool(int, std::string, std::string)> interruptCallback;  
}; 


using InterruptDescriptor = std::variant<
      UnixFileInterruptor
    , GpioInterruptor
    , HttpInterruptor
    #ifdef SDL_ENABLED
    , SdlIoInterruptor
    #endif

>;

#ifdef SDL_ENABLED
    void showAndFocusWindow(SDL_Window* window);
#endif

template<typename T>
concept Driveable = TypeDef::DEVTYPE<T>;

template <Driveable T>
class DeviceCore {
    private:
        std::vector<InterruptDescriptor> Idesc_list;
        TSQ<std::pair<std::thread::id, T>> queryQueue;
    
    protected:
        T states;
        std::shared_ptr<ADS7830> adc;
        
        // Interrupt watch
        void addFileIWatch(std::string &fileName, std::function<bool()> handler = sendState);
        void addGPIOIWatch(uint8_t gpioPort, std::function<bool(int, int, uint32_t)> handler);
        #ifdef SDL_ENABLED
        void addSDLIWatch(std::function<bool(SDL_Event* event)> handler);
        #endif
        std::shared_ptr<HttpListener> addEndpointIWatch(std::string endpoint, std::function<bool(int, std::string, std::string)> omar); 
        void writeQueryResult(T& states);
        T getLastQueryResult();


        friend class DeviceHandle;
}; 