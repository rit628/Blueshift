#pragma once

#include "libDM/DynamicMessage.hpp"
#include "include/ADC.hpp"
#include "include/HttpListener.hpp"
#include "libTSM/TSM.hpp"
#include "libnetwork/Protocol.hpp"
#include "Devices.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <boost/asio.hpp>
#include <chrono> 
#include "libnetwork/Connection.hpp"
#include <memory>
#include <shared_mutex>
#include <stop_token>
#include <sys/inotify.h>
#include <tuple>
#include <unistd.h> 
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <thread>
#ifdef SDL_ENABLED
#include <SDL3/SDL.h>
#endif


#define VOLATILITY_LIST_SIZE 10

/* 
    This file contains all the core device functions
    and utlity classes that can asist with the easy
    creation of devices. 


*/

using boost::asio::ip::tcp;
using boost::asio::ip::udp;

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

class DeviceInterruptor;

class DeviceHandle {
    private:
        using device_t = std::variant<
        std::monostate
        #define DEVTYPE_BEGIN(name, ...) \
        , Device::name
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        >;

        device_t device;

        void processStatesImpl(DynamicMessage& input);
    
    public:
        uint16_t id;
        uint32_t cooldown = 0;
        bool isTrigger;

        std::shared_mutex m;
        std::condition_variable_any cv;
        bool processing = false;
        bool watchersPaused = true;
        bool timersPaused = true;

        DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string> &config, std::shared_ptr<ADS7830> targetADC);
        void processStates(DynamicMessage input, uint16_t oblockId);
        void init(std::unordered_map<std::string, std::string> &config, std::shared_ptr<ADS7830> targetADC);
        void transmitStates(DynamicMessage &dmsg);
        void transmitDefaultStates(DynamicMessage &dmsg);
        DeviceKind getDeviceKind();
        std::vector<InterruptDescriptor>& getIdescList();
        std::optional<std::thread::id> getLatestCompletedHandlerId(std::stop_token stoken);

};

template<typename T>
class DeviceControlInterface {
    protected:
        DeviceHandle& device;
        std::shared_ptr<Connection> clientConnection;
        int ctl_code;
        int device_code;
    
    private:
        DeviceControlInterface() = delete;
        DeviceControlInterface(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code);
    
        friend T;
};

// Device Poller used for configuring polling rates dynamically
class DevicePoller : public DeviceControlInterface<DevicePoller> {
    private:
        struct DeviceTimer {
            int id;
            int poll_period = -1;
            std::chrono::milliseconds period_time;
            std::unordered_map<std::string, std::deque<float>> attr_history;
            std::unordered_map<std::string, float> vol_map;
            boost::asio::steady_timer timer;
            DevicePoller& manager;
        
            DeviceTimer(uint16_t id, int period, boost::asio::io_context& ctx, DevicePoller& manager);
            std::chrono::milliseconds getRemainingTime();
            float calculateStd(std::deque<float> &data);
            void calcVolMap();
            void timerCallback();
            void setPeriod(int newPeriod);
            void start();
            void pause();
            void resume();
        };

        boost::asio::io_context& ctx;
        std::unordered_map<uint16_t, DeviceTimer> timers;
        std::jthread timerManagerThread;

        void pauseTimers();
        void resumeTimers();
        void manageTimers(std::stop_token stoken);

    public: 
        DevicePoller(boost::asio::io_context& ctx, DeviceHandle& device, std::shared_ptr<Connection> cc, int ctl, int dev);
        DevicePoller(DevicePoller&& other);
        ~DevicePoller();

        void sendMessage(DeviceTimer& timer);
        void setPeriod(uint16_t timerId, int newPeriod);
        void createTimer(uint16_t timerId, int period);
        std::vector<uint16_t> getTimerIds();
        void startTimers();
}; 

/* 
    INTERRUPT STUFF
*/
class DeviceInterruptor : public DeviceControlInterface<DeviceInterruptor> {
    private:
        struct FileWatchDescriptor {
            int fd;
            int wd;
            std::string filename;
        };

        struct GpioWatchDescriptor {
            int port;
            void (*callback)(int, int, unsigned int, void*);
            void* callbackData;
        };

        #ifdef SDL_ENABLED
        struct SdlWatchDescriptor {
            SDL_EventFilter callback;
            void* callbackData;
        };
        #endif

        struct HttpWatchDescriptor{
            std::shared_ptr<HttpListener> listener; 
            std::function<bool(int64_t, string, string)> callback; 
            std::string endpoint;
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
        // Add Inotify thread blocking code here
        void IFileWatcher(std::stop_token stoken, std::string fname, std::function<bool()> handler);
        void IGpioWatcher(std::stop_token stoken, int portNum, std::function<bool(int, int , uint32_t)> handler);
        #ifdef SDL_ENABLED
        void ISdlWatcher(std::stop_token stoken, std::function<bool(SDL_Event*)> handler);
        #endif
        void IHttpWatcher(std::stop_token stoken, std::shared_ptr<HttpListener> server, std::string endpoint, std::function<bool(int64_t, string, string)> handler); 

    public: 
        DeviceInterruptor(boost::asio::io_context &in_ctx, DeviceHandle& targDev, std::shared_ptr<Connection> conex, int ctl, int dd);
        DeviceInterruptor(DeviceInterruptor&& other);
        ~DeviceInterruptor();
        // Setup Watcher Thread
        void setupThreads();
        void stopThreads();

}; 

class DeviceCursor : public DeviceControlInterface<DeviceCursor> {
    private:
        std::jthread queryWatcherThread;
        TSM<uint16_t, DynamicMessage> currentViews;
        TSM<std::thread::id, uint16_t> queryHandlers;

        void queryWatcher(std::stop_token stoken);
        void updateView(uint16_t oblockId);

    public:
        DeviceCursor(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code);
        DeviceCursor(DeviceCursor&& other);
        ~DeviceCursor();
        void initialize();
        DynamicMessage getLatestOblockView(uint16_t oblockId);
        void addQueryHandler(uint16_t oblockId);
        void awaitQueryCompletion(std::stop_token stoken);

};