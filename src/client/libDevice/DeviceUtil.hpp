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
#include <utility>
#include <variant>
#include <vector>
#include <thread>

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
        #define DEVTYPE_BEGIN(name) \
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

        DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string> &config);
        void processStates(DynamicMessage input);
        void init(std::unordered_map<std::string, std::string> &config);
        void transmitStates(DynamicMessage &dmsg);
        bool hasInterrupt();
        std::vector<Interrupt_Desc>& getIdescList();
};

// Device Timer used for configuring polling rates dynamically; 
class DeviceTimer{
    private: 
        DeviceHandle& device; 
        std::unordered_map<std::string, std::deque<float>> attr_history;  
        std::unordered_map<std::string, float> vol_map; 
        int ctl_code; 
        int device_code; 
        int timer_id; 
        // checks if the ticker is initialized
        bool ticker_init; 
        std::chrono::milliseconds period_time; 


        // Connection shard ptr
        std::shared_ptr<Connection> conex; 

        boost::asio::io_context &ctx; 
        boost::asio::steady_timer timer; 

        int poll_period = -1; 
        bool is_set = false; 

        std::chrono::milliseconds getRemainingTime();

    public: 
        DeviceTimer(boost::asio::io_context &in_ctx, DeviceHandle& device, std::shared_ptr<Connection> cc, int ctl, int dev, int id);
        ~DeviceTimer();

        void timerCallback();
        void setPeriod(int new_period);
        void Send(SentMessage &msg);
        float calculateStd(std::deque<float> &data);
        void calcVolMap();
        void sendData();
}; 

/* 
    INTERRUPT STUFF
*/
class DeviceInterruptor{
    private: 
        DeviceHandle& device; 
        std::shared_ptr<Connection> client_connection; 
        std::jthread watcherManagerThread;
        std::vector<std::jthread> globalWatcherThreads; 
        std::vector<std::tuple<int, int, std::string>> watchDescriptors; 
        int ctl_code; 
        int device_code;


        void sendMessage();
        void disableWatchers();
        void enableWatchers();
        void manageWatchers(std::stop_token stoken);
        // Add Inotify thread blocking code here
        void IFileWatcher(std::stop_token stoken, std::string fname, std::function<bool()> handler);
        void IGpioWatcher(std::stop_token stoken, int portNum, std::function<bool(int, int , uint32_t)> interruptHandle);
        
    public: 
        DeviceInterruptor(DeviceHandle& targDev, std::shared_ptr<Connection> conex, int ctl, int dd);
        DeviceInterruptor(DeviceInterruptor&& other);
        ~DeviceInterruptor();
        // Setup Watcher Thread
        void setupThreads();
        void stopThreads();

}; 
