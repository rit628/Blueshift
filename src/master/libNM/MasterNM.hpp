#pragma once
#include "libDM/DynamicMessage.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "include/Common.hpp"
#include <set>
#include <thread> 
#include "libTicker/Ticker.hpp"



using boost::asio::ip::tcp; 
using boost::asio::ip::udp; 
using DMM = DynamicMasterMessage;

class MasterNM{
    private: 
        std::vector<std::string> controller_list; 
        std::vector<std::string> device_list; 
        std::vector<std::string> oblock_list; 

        // Device descriptor info (mostly used for debugging maybe for other things): 
        std::unordered_map<std::string, DeviceDescriptor> dd_map; 


        // Controler configs
        std::unordered_map<std::string, DeviceConfigMsg> ctl_configs; 

        // Maps device/oblock alias names to devices
        std::unordered_map<std::string, uint16_t> device_alias_map;

        // Maps controller alias to devices
        std::unordered_map<std::string, uint16_t> controller_alias_map;

        // Oblock map
        std::unordered_map<std::string, uint16_t> oblock_alias_map;     

        // Networking stuff
        std::vector<std::shared_ptr<Connection>> temp_vector; 
        std::unordered_map<std::string, std::shared_ptr<Connection>> connection_map; 

        boost::asio::io_context master_ctx; 
        std::thread ctx_thread; 
        tcp::socket master_socket; 
        tcp::acceptor master_acceptor; 
        std::thread bcast_thread; 
        TSQ<OwnedSentMessage> in_queue; 

        // Ticker 
        MTicker tickerTable; 

        // Number of remaining processes to confirm
        int remConnections; 

        // This queue would come out of the master system
        TSQ<DMM> &EMM_in_queue; 
        TSQ<DMM> &EMM_out_queue; 

        // Reader thread (writer thread not necessary as its done by boost asio)
        std::thread readerThread; 
        std::thread updateThread; 
        bool in_operation; 

        void writeConfig(std::vector<OBlockDesc> &descs); 

        // Make a broadcast on 255.255.255.255 for device running on port 2988
        void broadcastIntro(); 

        // Connection related objects
        void listenForConnections(); 
        void messageClient(const std::string &controller, const SentMessage& sm); 
        void messageAllClients(const SentMessage &sm); 
        bool confirmClient(std::shared_ptr<Connection> &con_obj); 
        void onClientDisconnect(const std::string &controller); 
        void handleMessage(OwnedSentMessage &in_msg); 
    
        // Read in data and send it out; 
        void masterRead(); 
        void update(); 

        // Transfer the items
        void sendInitialTicker(std::shared_ptr<Connection> &client_con); 

        

    public:     
        MasterNM(std::vector<OBlockDesc> &descs, TSQ<DMM> &in_que, TSQ<DMM> &out_q); 
        ~MasterNM(); 
        bool start();
        void stop(); 
        void makeBeginCall();

  
}; 