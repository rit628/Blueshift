#pragma once

#include "include/Common.hpp"
#include "libDevice/DeviceUtil.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <shared_mutex>
#include <unordered_map>
#include "libDevice/include/ADC.hpp"
#include "libnetwork/Protocol.hpp"
#include "libnetwork/Connection.hpp"
#include "libtype/bls_types.hpp"
#include <set> 
#include <vector>

#define ADVERT_BC 

using boost::asio::ip::tcp; 
using boost::asio::ip::udp;

// numeric id types for different devices
using cont_int = uint16_t; 
using dev_int = uint16_t; 
using oblock_int = uint16_t; 

using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>; 

enum class ClientState{
    // Waiting for broadcast message from the master
    PRECONFIG, 
    // Configuration state of the client state
    CONFIGURATION, 
    // The Client is listening for state change messages
    IN_OPERATION, 
    // The client is in shutdown mode, so its going to sleep
    SHUTDOWN, 
};

// Scheduler Request State client side (Repalces the string val)
struct ClientSideReq{
    uint16_t requestorOblock; 
    uint16_t targetDevice; 
    int priority; 
    PROCSTATE ps;     
    int cyclesWaiting; 
    uint16_t ctl; 
};

// ClientSideReq comparator
struct ClientSideReqComp{
    bool operator()(const ClientSideReq& a, const ClientSideReq &b) const{
        return a.priority < b.priority; 
    }
}; 

struct ManagedDevice {
    DeviceHandle device;
    ControllerQueue<ClientSideReq, ClientSideReqComp> pendingRequests;
    std::pair<cont_int, oblock_int> owner;  
    ManagedDevice(TYPE dtype, std::unordered_map<std::string, std::string> &config, std::shared_ptr<ADS7830> targADC)
                        : device(dtype, config, targADC) {}
}; 

struct Advert_t{
    std::string MasterID; 
    std::unordered_map<std::string, std::string> bindingMap; 
    TYPE deviceType; 
    int args = 0; 
    std::vector<std::string> arguments;  
}; 


class Client{
    private: 

        /*
            Connection stuff
        */

        // Context must run in its own thread
        std::jthread ctxThread; 
        // Socket that is connected to server
        boost::asio::ip::tcp::socket client_socket;
        // The client object 
        std::shared_ptr<Connection> client_connection; 
        // Input Thread safe queue
        TSQ<OwnedSentMessage> in_queue; 
        // Input Thread 
        TSQ<OwnedSentMessage>client_in_queue; 
        // Omar 
        boost::asio::io_context client_ctx; 
    

        /*
            Blueshift Client Stuff
        */

        // Controller code (trade in for name)
        uint8_t controller_alias; 
       // Ticker Mutex; 
        std::shared_mutex ticker_mutex; 
        // Contains the list of known devices
        std::unordered_map<int, ManagedDevice> deviceList; 
        // thread pool for devices state change
        boost::asio::thread_pool threadPool;
        std::unordered_map<dev_int, boost::asio::strand<boost::asio::thread_pool::executor_type>> deviceStrands;
        std::unordered_map<dev_int, std::unordered_map<oblock_int, boost::asio::strand<boost::asio::thread_pool::executor_type>>> cursorViewStrands;
        
        // client name used to identify controller
        std::string client_name; 
        // Listens for incoming message and places it into the spot
        std::jthread listenerThread; 
        // use to keep track of the state 
        ClientState curr_state; 
        // sends a callback for a device
        void sendMessage(uint16_t device, Protocol prot, bool fromint, oblock_int oint, bool write_self);  
        // Send a message
        void send(SentMessage &msg); 
        // Updates the ticker table
        void updateTicker(); 
        // Temp timers 
        std::unordered_map<uint16_t, std::vector<Timer>> start_timers;
        
        // Error Sender: 
        std::unique_ptr<GenericBlsException> genBlsException; 

        // Ticker table
        std::unordered_map<uint16_t, std::reference_wrapper<DevicePoller>> client_ticker;
        
        /*
            State management information
        */
        std::vector<DevicePoller> pollers;
        std::vector<DeviceInterruptor> interruptors;
        std::unordered_map<uint16_t, DeviceCursor> cursors;
        std::shared_ptr<ADS7830> adc;


    public: 
        // Client constructor
        Client(std::string name, boost::asio::io_context &ctx, udp::endpoint &master_endpt); 
       
        // Dispatches messages to jobs when recieved 
        void listener(std::stop_token stoken);

        // shutdown
        void shutdown();

        /*
            Connection stuff
        */
    
        void initializeClient(udp::endpoint& master_endpt); 
        bool attemptConnection(boost::asio::ip::address master_address, std::string init_ctx);
        bool isConnected(); 
        bool disconnect(); 
        TSQ<OwnedSentMessage>& getInQueue(); 

        // Later Features of other types of connections / connect to external apis: 
        bool connectTo(const std::string &endpt, uint16_t port); 
}; 


// Holds several clients
class ConnectionManager{
    private: 
        // Connection Manager
        const int broadcast_pulse = 2500; 
        std::vector<Advert_t> advertList; 
        std::vector<std::string> advertStr; 
        std::vector<std::jthread> clientActiveThreads; 
        std::string clientName;
        std::vector<std::unique_ptr<Client>> clients; 
        boost::asio::io_context clientCtx; 
        boost::asio::ip::udp::socket bc_listener;
        std::mutex advertMutex; 
        std::atomic_bool foma = false; 
        

        std::unordered_map<std::string, std::unique_ptr<Client>> clientMaps; 

    public: 
        ConnectionManager(std::string &clientName, std::vector<Advert_t> &ads);
        void begin(); 

        void startAdverts();

        // Broadcast for controllers declared in the language
        void startListener(); 
        // Broadcast for devices advertised by the s

        // Initiator parser
        void handleInitiatorString(std::string &str, udp::endpoint &endpt); 

        void broadcastAdvert(std::vector<char> &bytestream); 
        void connectionListen();
         
}; 