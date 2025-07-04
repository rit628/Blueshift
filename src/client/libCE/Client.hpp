#pragma once

#include "include/Common.hpp"
#include "libDevice/DeviceUtil.hpp"
#include <memory>
#include <thread>
#include <shared_mutex>
#include <unordered_map>
#include "libnetwork/Protocol.hpp"
#include "libnetwork/Connection.hpp"

using boost::asio::ip::tcp; 
using boost::asio::ip::udp; 

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

        /*
            Blueshift Client Stuff
        */

        // Controller code (trade in for name)
        uint8_t controller_alias; 
       // Ticker Mutex; 
        std::shared_mutex ticker_mutex; 
        // Contains the list of known devices
        std::unordered_map<int, DeviceHandle> deviceList;     
        // client name used to identify controller
        std::string client_name; 
        // Listens for incoming message and places it into the spot
        std::jthread listenerThread; 
        // use to keep track of the state 
        ClientState curr_state; 
        // sends a callback for a device
        void sendMessage(uint16_t device, Protocol prot, bool );  
        // Send a message
        void send(SentMessage &msg); 
        // Updates the ticker table
        void updateTicker(); 
        // Temp timers 
        std::vector<Timer> start_timers; 
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