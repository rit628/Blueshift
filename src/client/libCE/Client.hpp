#pragma once

#include "include/Common.hpp"
#include "libDevice/DeviceUtil.hpp"
#include <memory>
#include <queue>
#include <thread>
#include <shared_mutex>
#include <unordered_map>
#include "libnetwork/Protocol.hpp"
#include "libnetwork/Connection.hpp"
#include "libClientGateway/ClientGateway.hpp"
#include <set> 

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
    ManagedDevice(TYPE dtype, std::unordered_map<std::string, std::string> &config)
                        : device(dtype, config) {}
}; 

class Client{
    private: 

        /*
            Connection stuff
        */

        // Sets up the boost asio context: 
        boost::asio::io_context client_ctx; 
        // Context must run in its own thread
        std::jthread ctxThread; 
        // Socket that listens for broadcast: 
        boost::asio::ip::udp::socket bc_socket; 
        // Socket that is connected to server
        boost::asio::ip::tcp::socket client_socket;
        // The client object 
        std::shared_ptr<Connection> client_connection; 
        // Input Thread safe queue
        TSQ<OwnedSentMessage> connection_in_queue; 
        // Input Thread 
        TSQ<OwnedSentMessage>client_in_queue; 


        /*
            Blueshift Client Stuff
        */
  
        uint8_t controller_alias;   
        std::shared_mutex ticker_mutex; 
        std::unordered_map<int, ManagedDevice> deviceList;     
        std::string client_name; 
        std::jthread listenerThread;  
        ClientState curr_state; 
        void sendMessage(uint16_t device, Protocol prot, bool fromint, oblock_int oint, bool write_self);  
        void send(SentMessage &msg); 
        void updateTicker(); 
        std::vector<Timer> start_timers;  
        std::unique_ptr<GenericBlsException> genBlsException; 

        /*
            State management information
        */

        std::unordered_map<uint16_t, DeviceTimer> client_ticker;
        std::vector<DeviceInterruptor> interruptors;

        /*  
            Client EU and routing for decentralization
        */
        std::unique_ptr<ClientEM> ClientExec; 
        std::unordered_map<uint16_t, std::vector<std::string>> devRouteMap; 
        std::unordered_map<std::string, int> devAliasMap;
        jthread execManThread; 


    public: 
        // Client constructor
        Client(std::string name); 
        // Start the client (broadcast protocol + others)
        void start(); 

        // Dispatches messages to jobs when recieved 
        void listener(std::stop_token stoken);

        // shutdown
        void shutdown();

        /*
            Connection stuff
        */
    
        void broadcastListen(); 
        bool attemptConnection(boost::asio::ip::address master_address);
        bool isConnected(); 
        bool disconnect(); 
        TSQ<OwnedSentMessage>& getInQueue(); 

        // Later Features of other types of connections / connect to external apis: 
        bool connectTo(const std::string &endpt, uint16_t port); 


}; 