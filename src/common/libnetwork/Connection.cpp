 #include "Connection.hpp"
#include "boost/asio/write.hpp"
#include "boost/system/detail/error_code.hpp"
#include "libnetwork/Protocol.hpp"
#include <array>
#include <cstddef>
 
Connection::Connection(boost::asio::io_context &in_ctx, 
tcp::socket socket ,Owner own_type, TSQ<OwnedSentMessage> &in_msg, std::string &ip_addr) 
: ctx(in_ctx), socket(std::move(socket)), in_queue(in_msg)
    {
        for(int i = 0 ; i < IN_MSGSIZE ; i++){
            this->in_tickets.push_back(i); 
        }

        own = own_type;
        ip = ip_addr; 
    };



void Connection::connectToClient(){
    if(this->own == Owner::MASTER){
        if(this->socket.is_open()){
            this->readHeader(); 
        }
    }
}

void Connection::setName(std::string& cname){
    this->client_name = cname; 
}

void Connection::connectToMaster(tcp::endpoint master_ep ,std::string &name){
    if(this->own == Owner::CLIENT){
        this->socket.async_connect(master_ep, 
        [name, this](boost::system::error_code ec){
            if(!ec){
                // Start by sending the name when the connection is made
                SentMessage new_message; 
                DynamicMessage dmsg; 
                std::string omar(name); 
                dmsg.createField("__CONTROLLER_NAME__", omar);
                
                new_message.header.prot = Protocol::CONFIG_NAME; 
                new_message.body = dmsg.Serialize(); 
                new_message.header.body_size = new_message.body.size();

                // Send message name and recieve
                this->send(new_message); 

                // begin reading header
                this->readHeader(); 
            }

        }); 
    }
}

/* 
    To add later maybe
*/

void Connection::connectToServer(boost::asio::ip::tcp::resolver::results_type &results){
    boost::asio::async_connect(this->socket, results, 
    [](boost::system::error_code ec, tcp::endpoint endpt){
        if(!ec){
            std::cout<<"Read Header"<<std::endl; 
        }
    });
}

bool Connection::disconnect(){
    if(this->isConnected()){
        boost::asio::post([this](){this->socket.close();}); 
    }

    return false; 
}

bool Connection::isConnected() const{ 
    return this->socket.is_open(); 
}

std::string& Connection::getIP(){
    return this->ip; 
}

void Connection::send(const SentMessage &sm){
    boost::asio::post(this->ctx, [this, sm](){
        boost::asio::async_write(this->socket, std::array{
            boost::asio::buffer(&sm.header, sizeof(SentHeader)),
            boost::asio::buffer(sm.body.data(), sm.header.body_size)
        },
        [](boost::system::error_code ec, size_t size) {
            if (ec) {
                std::cerr << "Error Writing Message: " << ec.message() << std::endl;
            }
        });
    });
}

std::string& Connection::getName(){
    return this->client_name; 
}

void Connection::readHeader(){
    int index = this->in_tickets.back();
    this->in_tickets.pop_back();

    auto& header = this->in_messageBuffer.at(index).header;

    boost::asio::async_read(this->socket, boost::asio::buffer(&header, sizeof(SentHeader)) ,
    [this, index](boost::system::error_code ec, size_t len){
        if(!ec){
            auto& bodySize = this->in_messageBuffer.at(index).header.body_size;
            this->in_messageBuffer.at(index).body.resize(bodySize); 
            this->readBody(index);
        }
        else{
            std::cerr<<"READ HEADER ERROR: "<<ec.message()<<std::endl; 
            if(this->own == Owner::CLIENT){
                std::cout<<"Client Connection detected, reverting to search mode!"<<std::endl; 
                

            }
            else{
                std::cout<<"Master system disconnect, reverting to search mode!"<<std::endl; 
            }



            //this->socket.close(); 
        }
    }); 
}


void Connection::readBody(int index){
    auto& readMsg = this->in_messageBuffer.at(index);
    boost::asio::async_read(this->socket, boost::asio::buffer(readMsg.body.data(), readMsg.header.body_size), 
    [this, index](boost::system::error_code ec, size_t len){ 
        if(!ec){
            this->addToQueue(index); 
        }
        else{
            std::cerr<<"Connection: READ BODY"<<ec.message()<<std::endl; 
        }
    }); 
}

void Connection::addToQueue(int val){
    if(this->own == Owner::MASTER){
        this->in_queue.write({.connection=this->shared_from_this(), .sm=this->in_messageBuffer.at(val)}); 
    }
    else if(this->own == Owner::CLIENT){
        this->in_queue.write({.connection=nullptr, .sm=this->in_messageBuffer.at(val)}); 
    }
    
    this->in_tickets.push_front(val); 
    this->readHeader(); 
}

