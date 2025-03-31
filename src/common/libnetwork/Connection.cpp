 #include "Connection.hpp"
 #define IN_MSGSIZE 256
 
Connection::Connection(boost::asio::io_context &in_ctx, 
tcp::socket socket ,Owner own_type, TSQ<OwnedSentMessage> &in_msg, std::string &ip_addr) 
: ctx(in_ctx), socket(std::move(socket)), in_queue(in_msg)
    {
        this->in_messageBuffer.reserve(IN_MSGSIZE);
        this->in_messageBuffer.resize(IN_MSGSIZE);
        for(int i = 0; i < IN_MSGSIZE; i++){
            this->tickets.push_back(i); 
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
    [this](boost::system::error_code ec, tcp::endpoint endpt){
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
        bool write_empty = this->out_queue.isEmpty(); 
        this->out_queue.write(sm); 

        // if empty before new item added, then add the object
        if(write_empty){   
            writeHeader(); 
        }
    });
}

std::string& Connection::getName(){
    return this->client_name; 
}

void Connection::readHeader(){
    int index = this->tickets.back(); 
    this->tickets.pop_back(); 

    boost::asio::async_read(this->socket, boost::asio::buffer(&this->in_messageBuffer[index].header, sizeof(SentHeader)) ,
    [this, index](boost::system::error_code ec, size_t len){
        if(!ec){
            if(this->in_messageBuffer[index].header.body_size > 0){
                int newSize = this->in_messageBuffer[index].header.body_size; 
                this->in_messageBuffer[index].body.resize(newSize); 
                this->readBody(index);
            }
            else{
                addToQueue(index);  
            }
        }
        else{
            std::cerr<<"READ HEADER ERROR: "<<ec.message()<<std::endl; 
            this->socket.close(); 
        }
    }); 
}


void Connection::readBody(int index){
    boost::asio::async_read(this->socket, boost::asio::buffer(this->in_messageBuffer[index].body.data(), this->in_messageBuffer[index].header.body_size), 
    [this, index](boost::system::error_code ec, size_t len){ 
        if(!ec){
            addToQueue(index); 
        }
        else{
            std::cerr<<"Connection: READ BODY"<<ec.message()<<std::endl; 
        }
    }); 
}

void Connection::writeHeader(){
    auto frontQueue = this->out_queue.peek(); 
    boost::asio::async_write(this->socket, boost::asio::buffer(&frontQueue.header, sizeof(SentHeader)), 
    [frontQueue, this](boost::system::error_code ec, size_t size){
        if(!ec){
            if(frontQueue.header.body_size > 0){
                writeBody(); 
            }
            else{
                // Pop and restart queue
                this->out_queue.read(); 
                if(!this->out_queue.isEmpty()){
                    writeHeader(); 
                }
            }
        }
        else{
            std::cerr<<"Error Writing Header: "<<ec.message()<<std::endl; 

        }
    }); 

}

void Connection::writeBody(){
    auto frontQueue = this->out_queue.peek();
    boost::asio::async_write(this->socket, boost::asio::buffer(frontQueue.body.data(), frontQueue.header.body_size), 
    [&, this](boost::system::error_code ec, size_t len){
        if(!ec){    
            // pop the latest message off the stack and write header if no empty
            this->out_queue.read(); 
            if(!this->out_queue.isEmpty()){
                writeHeader(); 
            }
        }   
        else{
            std::cerr<<"Error writing header: "<<ec.message()<<std::endl; 
           
        }
    }); 
}

void Connection::addToQueue(int val){
    if(this->own == Owner::MASTER){
        this->in_queue.write({this->shared_from_this(), this->in_messageBuffer[val]}); 
    }
    else if(this->own == Owner::CLIENT){
        this->in_queue.write({nullptr, this->in_messageBuffer[val]}); 
    }
    
    this->tickets.push_back(val); 
    this->readHeader(); 
}

