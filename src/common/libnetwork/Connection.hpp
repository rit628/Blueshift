#pragma once

#include <boost/asio.hpp>
#include "libTSQ/TSQ.hpp"
#include "Protocol.hpp"

using boost::asio::ip::tcp; 

enum class Owner{
    CLIENT,
    MASTER, 
}; 

struct OwnedSentMessage; 

class Connection : public enable_shared_from_this<Connection>{
    private: 
        boost::asio::io_context &ctx; 
        tcp::socket socket;
        Owner own; 
        std::string ip;
        std::string client_name; 
        SentMessage currMessage; 

        // Confirm Connection (checks if the targeted device is the right device)
        bool connectionConfirm; 

        // Queues
        TSQ<SentMessage> out_queue; 
        TSQ<OwnedSentMessage> &in_queue; 

        // Async Reader Writer functions: 
        void addToQueue(); 
        void readHeader(); 
        void readBody(); 
        void writeHeader(); 
        void writeBody(); 

    public: 

        Connection(boost::asio::io_context &in_ctx, tcp::socket i_socket, Owner own_type, TSQ<OwnedSentMessage> &in_msg, std::string &ip_addr);
        ~Connection() = default; 


        void connectToMaster(tcp::endpoint endpoints, std::string &name); 
        void connectToClient(); 
        bool disconnect(); 
        bool isConnected() const; 

        std::string& getIP(); 
        void send(const SentMessage &sm); 
        std::string& getName(); 
        void setName(std::string& cname); 

        // Used to connect Master to external APIs (phase 3)
        void connectToServer(boost::asio::ip::tcp::resolver::results_type &results); 
}; 


//Actual object
struct OwnedSentMessage{
    std::shared_ptr<Connection> connection; 
    SentMessage sm; 
}; 