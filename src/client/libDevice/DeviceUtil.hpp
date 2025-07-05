#pragma once

#include "libDM/DynamicMessage.hpp"
#include "libnetwork/Protocol.hpp"
#include "Devices.hpp"
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <boost/asio.hpp>
#include <chrono> 
#include "libnetwork/Connection.hpp"
#include <memory>
#include <mutex>
#include <stop_token>
#include <sys/inotify.h>
#include <tuple>
#include <unistd.h> 
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <thread>
#include <boost/lockfree/queue.hpp>
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

enum class DeviceKind {
    POLLING,
    INTERRUPT,
    CURSOR
};

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
        bool isTrigger;

        std::mutex m;
        std::condition_variable_any cv;
        bool processing = false;
        bool watchersPaused = false;
        boost::lockfree::queue<uint16_t> modifiedOblockIds;

        DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string> &config, uint16_t sendInterrupt);
        void processStates(DynamicMessage input, uint16_t oblockId = 0);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
        DeviceKind getDeviceKind();
        std::vector<InterruptDescriptor>& getIdescList();
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
    
    public:
        void sendMessage(uint16_t oblockId = 0);
        
        friend T;
};

// Device Timer used for configuring polling rates dynamically; 
class DeviceTimer : public DeviceControlInterface<DeviceTimer> {
    private: 
        std::unordered_map<std::string, std::deque<float>> attr_history;  
        std::unordered_map<std::string, float> vol_map; 
        int timer_id;
        // checks if the ticker is initialized
        bool ticker_init; 
        std::chrono::milliseconds period_time; 

        boost::asio::io_context &ctx;
        boost::asio::steady_timer timer;

        int poll_period = -1; 
        bool is_set = false; 

        std::chrono::milliseconds getRemainingTime();

    public: 
        DeviceTimer(boost::asio::io_context &in_ctx, DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code, int timer_id);
        ~DeviceTimer();

        void timerCallback();
        void setPeriod(int new_period);
        float calculateStd(std::deque<float> &data);
        void calcVolMap();
        void sendMessage(uint16_t oblockId = 0);
}; 

/* 
    INTERRUPT STUFF
*/
class DeviceInterruptor : public DeviceControlInterface<DeviceInterruptor> {
    private: 
        std::jthread watcherManagerThread;
        std::vector<std::jthread> globalWatcherThreads; 
        std::vector<std::tuple<int, int, std::string>> watchDescriptors; 

        void disableWatchers();
        void enableWatchers();
        void manageWatchers(std::stop_token stoken);
        // Add Inotify thread blocking code here
        void IFileWatcher(std::stop_token stoken, std::string fname, std::function<bool()> handler);
        void IGpioWatcher(std::stop_token stoken, int portNum, std::function<bool(int, int , uint32_t)> handler);
        #ifdef SDL_ENABLED
        void ISdlWatcher(std::stop_token stoken, std::function<bool(SDL_Event*)> handler);
        #endif
        
    public: 
        DeviceInterruptor(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code);
        DeviceInterruptor(DeviceInterruptor&& other);
        ~DeviceInterruptor();
        // Setup Watcher Thread
        void setupThreads();
        void stopThreads();
        void sendMessage(uint16_t oblockId = 0);

};

class DeviceCursor : public DeviceControlInterface<DeviceCursor> {
    private:
        std::jthread queryWatcherThread;

        void queryWatcher(std::stop_token stoken);

    public:
        DeviceCursor(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code);
        DeviceCursor(DeviceCursor&& other);
        ~DeviceCursor();
        void initialize();
        void sendMessage(uint16_t oblockId);

};
