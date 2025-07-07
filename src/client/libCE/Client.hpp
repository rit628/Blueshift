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


// Receives the queue for each controller
struct ControllerQueue{
    private:  
        std::priority_queue<ClientSideReq, std::vector<ClientSideReq>, ClientSideReqComp> schepq; 

    public: 

        bool currOwned = false; 
        // The device owner is identified by the ctl_id, oblock_int
        std::pair<cont_int, oblock_int> owner;     
        std::priority_queue<ClientSideReq, std::vector<ClientSideReq>, ClientSideReqComp>& getQueue(){
            return this->schepq; 
        }
};  


struct ManagedDevice {
    DeviceHandle device;
    ControllerQueue pendingRequests;
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
        TSQ<OwnedSentMessage> in_queue; 
        // Input Thread 
        TSQ<OwnedSentMessage>client_in_queue; 


        /*
            Blueshift Client Stuff
        */

        // Controller code (trade in for name)
        uint8_t controller_alias; 
       // Ticker Mutex; 
        std::shared_mutex ticker_mutex; 
        // Contains the list of known devices
        std::unordered_map<int, ManagedDevice> deviceList;     
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
        std::vector<Timer> start_timers; 
        // Run the client listener
        
        // Error Sender: 
        std::unique_ptr<GenericBlsException> genBlsException; 

        /*
            State management information
        */
                
        // Ticker table
        std::unordered_map<uint16_t, DeviceTimer> client_ticker;
        std::vector<DeviceInterruptor> interruptors;


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