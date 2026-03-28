#pragma once
#include "device_control.hpp"
#ifdef SDL_ENABLED
#include <SDL3/SDL.h>
#endif

class DeviceInterruptor : public DeviceControlInterface<DeviceInterruptor> {
    private:
        struct SynchronizationData {
            std::condition_variable_any cv;
            std::mutex m;
            bool handlerSignal = false;
        };

        struct FileWatchDescriptor {
            std::filesystem::path file;
            std::function<void()> callback;
            SynchronizationData syncPrimitives;
        };

        struct GpioWatchDescriptor {
            int port;
            void (*callback)(int, int, unsigned int, void*);
            std::pair<SynchronizationData, std::function<bool(int, int, uint32_t)>> callbackData{std::piecewise_construct, {}, {}};
        };

        #ifdef SDL_ENABLED
        struct SdlWatchDescriptor {
            SDL_EventFilter callback;
            std::pair<SynchronizationData, std::function<bool(SDL_Event*)>> callbackData{std::piecewise_construct, {}, {}};
        };
        #endif

        struct HttpWatchDescriptor {
            std::shared_ptr<HttpListener> listener; 
            std::string endpoint;
            std::function<bool(int64_t, std::string, std::string)> callback; 
            SynchronizationData syncPrimitives;
        }; 

        using WatchDescriptor = std::variant<
              FileWatchDescriptor
            , GpioWatchDescriptor
            #ifdef SDL_ENABLED
            , SdlWatchDescriptor
            #endif
            , HttpWatchDescriptor
        >;

        std::jthread watcherManagerThread;
        std::vector<std::jthread> globalWatcherThreads;
        std::vector<WatchDescriptor> watchDescriptors;
        boost::asio::steady_timer cooldownTimer;
        std::atomic_bool running = true;


        void restartCooldown();
        bool inCooldown();
        void sendMessage();
        void disableWatchers();
        void enableWatchers();
        void manageWatchers(std::stop_token stoken);
        void InterruptWatcher(std::stop_token stoken, SynchronizationData& syncPrimitives);
        
        SynchronizationData& addFileWatch(std::string filename, std::function<bool()> handler, FileWatchDescriptor& wdesc);
        SynchronizationData& addGpioWatch(int portNum, std::function<bool(int, int, uint32_t)> handler, GpioWatchDescriptor& wdesc);
        #ifdef SDL_ENABLED
        SynchronizationData& addSdlWatch(std::function<bool(SDL_Event*)> handler, SdlWatchDescriptor& wdesc);
        #endif
        SynchronizationData& addHttpWatch(std::shared_ptr<HttpListener> server, std::string endpoint, std::function<bool(int64_t, std::string, std::string)> handler, HttpWatchDescriptor& wdesc);

    public: 
        DeviceInterruptor(boost::asio::io_context& in_ctx, DeviceHandle& targDev, std::shared_ptr<Connection> conex, int ctl, int dd);
        DeviceInterruptor(DeviceInterruptor&& other);
        ~DeviceInterruptor();
        void setupThreads();
        void stopThreads();
}; 