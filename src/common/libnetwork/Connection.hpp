#pragma once

#include <array>
#include <boost/asio.hpp>
#include "libTSQ/TSQ.hpp"
#include "Protocol.hpp"
#include <memory>
#include <list>

#define IN_MSGSIZE 256

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
        //SentMessage currMessage; 

        // in_message buffer 
        std::array<SentMessage, IN_MSGSIZE> in_messageBuffer; 
        std::list<int> in_tickets; 

        // Confirm Connection (checks if the targeted device is the right device)
        bool connectionConfirm; 

        // Queues
        TSQ<SentMessage> out_queue; 
        TSQ<OwnedSentMessage> &in_queue; 

        // Async Reader Writer functions: 
        void addToQueue(int index); 
        void readHeader(); 
        void readBody(int index); 

    public: 

        Connection(boost::asio::io_context &in_ctx, tcp::socket i_socket, Owner own_type, TSQ<OwnedSentMessage> &in_msg, std::string &ip_addr);
        ~Connection() = default; 


        void connectToMaster(tcp::endpoint endpoints, std::string &name); 
        void connectToClient(); 
        bool disconnect(); 
        bool isConnected() const; 

        std::string& getIP(); 
        void send(SentMessage sm); 
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


class BlsExceptionClass : public std::exception{
    std::string msg; 
    ERROR_T error_t; 

    public: 
        explicit BlsExceptionClass(const std::string& message, ERROR_T error_type) :
        msg(message), error_t(error_type) {}
        const char* what() const noexcept override{
            return msg.c_str(); 
        }
        ERROR_T type() {return this->error_t;}
        bool isFatal() {return this->error_t < ERROR_T::FATAL_ERROR;}
}; 


class GenericBlsException{

    private: 
        std::shared_ptr<Connection> ConObj; 
        Owner role; 
        std::string message; 

    public: 
        GenericBlsException(std::shared_ptr<Connection> conny, Owner role)
        {
            this->ConObj = conny; 
            this->role = role; 
        }

        // Generic Exception 
        void SendGenericException(std::string errorMsg, ERROR_T ec){
            SentMessage sm; 
            std::string caller = "MASTER";
            sm.header.prot = Protocol::MASTER_ERROR;  

            if(this->role == Owner::CLIENT){
               caller = ConObj->getName(); 
               sm.header.prot = Protocol::CLIENT_ERROR; 
            }
            DynamicMessage dmsg; 

            errorMsg = "FROM " + caller + ": " +errorMsg; 
            dmsg.createField("message", errorMsg);
            sm.body = dmsg.Serialize(); 
            sm.header.body_size = sm.body.size();
            sm.header.ec = ec; 
            this->ConObj->send(sm); 
        }

        // Handles the reception of a message
        void RecieveGenericException(SentMessage &sm){
            DynamicMessage dmsg; 
            dmsg.Capture(sm.body); 
            std::string err_msg; 
            dmsg.unpack("message", err_msg); 
            throw BlsExceptionClass(err_msg, sm.header.ec); 
        }
}; 