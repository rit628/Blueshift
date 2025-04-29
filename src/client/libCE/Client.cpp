#include "Client.hpp"
#include "libnetwork/Protocol.hpp"

Client::Client(std::string c_name): bc_socket(client_ctx, udp::endpoint(udp::v4(), BROADCAST_PORT)), client_socket(client_ctx){
    this->client_name = c_name; 
    std::cout<<"Client Created: " << c_name <<std::endl; 
}

void Client::start(){
    std::cout<<"Starting Client"<<std::endl; 
    this->broadcastListen(); 
}

void Client::sendMessage(uint16_t deviceCode, Protocol type, bool fromInt){
    // Write code for a callback
    SentMessage sm; 
    DynamicMessage dmsg; 

    // Get the latest state from the dmsg
 
    this->deviceList.at(deviceCode).transmitStates(dmsg); 

    // make message
    sm.body = dmsg.Serialize(); 
    sm.header.ctl_code = this->controller_alias;
    sm.header.prot = type; 
    sm.header.device_code = deviceCode; 
    sm.header.timer_id = -1; 
    sm.header.volatility = -1; 
    sm.header.body_size = sm.body.size(); 
    sm.header.fromInterrupt = fromInt; 
    
    this->client_connection->send(sm); 
}



void Client::listener(){
    while(true){

        auto inMsg = this->in_queue.read().sm; 
        
        Protocol ptype = inMsg.header.prot; 

        DynamicMessage dmsg; 
        dmsg.Capture(inMsg.body);

        if(ptype == Protocol::CONFIG_OK){
            this->curr_state = ClientState::IN_OPERATION; 
        }
        else if(ptype == Protocol::SHUTDOWN){
            // Shutown and end the listener thread; 
            this->curr_state = ClientState::SHUTDOWN; 
            return; 
        } 
        else if(ptype == Protocol::CONFIG_INFO){
            this->curr_state = ClientState::CONFIGURATION; 

            std::vector<uint16_t> device_alias; 
            std::vector<TYPE> device_types; 
            std::vector<std::unordered_map<std::string, std::string>> srcs;  
            std::vector<uint16_t> triggerList;
            uint8_t controller_alias = inMsg.header.ctl_code; 

            dmsg.unpack("__DEV_ALIAS__", device_alias);
            dmsg.unpack("__DEV_TYPES__", device_types); 
            dmsg.unpack("__DEV_PORTS__", srcs); 
            dmsg.unpack("__DEV_INIT__", triggerList); 

            this->controller_alias = controller_alias; 

            // Check that all vectors are of equal size and more than 0 devices are configured: 
            int size = device_alias.size(); 
            bool b = device_types.size() == size; 
            bool c = srcs.size() == size; 
            bool d = triggerList.size() == size; 
            

            if(!(b && c  && d)){
                throw std::invalid_argument("Config vectors of different sizes what!"); 
            }

            for(int i = 0; i < size; i++){
                std::cout<<"Device a: "<<device_alias[i]<<" trigger value: "<<triggerList[i]<<std::endl; 
                deviceList.try_emplace(device_alias[i], device_types[i], srcs[i], triggerList[i]);
            } 

            // Check item: 
            SentMessage sm; 
            sm.header.body_size = 0;
            sm.header.prot = Protocol::CONFIG_OK; 
            sm.header.ctl_code = this->controller_alias; 
            this->client_connection->send(sm); 
        

            std::cout<<"Client side handshake complete!"<<std::endl; 

        }
        else if(ptype == Protocol::STATE_CHANGE){
        
            if(this->curr_state == ClientState::IN_OPERATION){
                auto dev_index = inMsg.header.device_code; 
    
                auto state_change = std::thread([dev_index, dmsg = std::move(dmsg), this](){
                try{
                    // std::cout << "processStates begin" << std::endl;
                    this->deviceList.at(dev_index).processStates(dmsg);
                    //Translation of the callback happens at the network manage
                    this->sendMessage(dev_index, Protocol::CALLBACK, false); 
                }
                catch(std::exception(e)){
                    std::cout<<e.what()<<std::endl; 
                }
                });

                state_change.detach(); 


            }
            else{
                std::cout<<"Cannot process state change until configuration is complete"<<std::endl; 
            }
        }
        else if(ptype == Protocol::TICKER_INITIAL){
            // Conifigure the the dynamic timers
            if(inMsg.header.body_size > 0){
                std::vector<Timer> all_timers; 
                dmsg.unpack("__TICKER_DATA__", all_timers); 
                this->start_timers = all_timers; 
            }
        }
        else if(ptype == Protocol::TICKER_UPDATE){
        
            std::vector<Timer> tickerUpdate; 

            dmsg.unpack("__TICKER_UPDATE__", tickerUpdate); 

            for(Timer &new_timer : tickerUpdate){
                if(!new_timer.const_poll){
                    if(this->client_ticker.find(new_timer.id) == this->client_ticker.end()){
                        std::cout<<"UH OH: "<<new_timer.id<<std::endl; 
                    }
                    this->client_ticker.at(new_timer.id).setPeriod(new_timer.period);
                }
                else{
                    std::cerr<<"Constant poll not supposed to be sent by update protocol"<<std::endl; 
                }

            }

        }
        else if(ptype == Protocol::BEGIN){

            std::cout<<"CLIENT: Beginning sending process"<<std::endl; 
            this->curr_state = ClientState::IN_OPERATION; 

            // Begin the timers only when the call is made
            for(Timer &timer : this->start_timers){
                auto& device = this->deviceList.at(timer.device_num); 

                if(!device.hasInterrupt()){
                    std::cout<<"build timer with period: "<<timer.period<<std::endl;
                    this->client_ticker.try_emplace(timer.id, this->client_ctx, device, this->client_connection, this->controller_alias, timer.device_num, timer.id);
                    this->client_ticker.at(timer.id).setPeriod(timer.period);
                    if(!device.isTrigger()){
                        sendMessage(timer.device_num, Protocol::SEND_STATE, false); 
                    }
                }
            }

             // populate the Device interruptors; 
            for(auto& pair : this->deviceList){
                
                auto& dev = pair.second;
                auto dev_id = pair.first;  
             
                if(dev.hasInterrupt()){
                    // Organizes the device interrupts
                    std::cout<<"Interrupt created!"<<std::endl; 
                    auto& omar = this->interruptors.emplace_back(dev, this->client_connection, this->global_interrupts, this->controller_alias, dev_id);
                    omar.setupThreads();
                    if(!dev.isTrigger()){
                        std::cout<<"Sending Initial State"<<std::endl; 
                        sendMessage(dev_id, Protocol::SEND_STATE, true); 
                    }
                }   
            }

        }
        else{
            std::cout<<"Unknown protocol message!"<<std::endl; 
        }
    }
    
}

void Client::broadcastListen(){
    while(1){
        std::cout<<"CLIENT: waiting for client connection"<<std::endl; 
        std::vector<char> buffer(MAX_NAME_LEN); 
        udp::endpoint master_endpoint; 
        size_t bytes_recv = this->bc_socket.receive_from(boost::asio::buffer(buffer.data(), buffer.size()), master_endpoint); 
        buffer.resize(bytes_recv); 

        std::string attempt_name = std::string(buffer.data()); 
        if(attempt_name == this->client_name){
            auto master_address = master_endpoint.address().to_string(); 
            this->client_connection = std::make_shared<Connection>(
                this->client_ctx, tcp::socket(this->client_ctx), Owner::CLIENT, this->in_queue, master_address
            ); 

            this->attemptConnection(master_endpoint.address()); 
            break;
        }
    }
}

bool Client::attemptConnection(boost::asio::ip::address master_address){
    try{
        tcp::endpoint master_endpoint(master_address, MASTER_PORT); 

        this->client_connection->connectToMaster(master_endpoint, this->client_name);

        this->listenerThread = std::thread([this](){this->listener();});
        this->ctxThread = std::thread([this](){this->client_ctx.run();});   

        if(this->listenerThread.joinable()){
            this->listenerThread.join(); 
        }

        // Join the threads to maintian concurrency
        if(this->ctxThread.joinable()){
            this->ctxThread.join(); 
        }

        std::cout<<this->client_name + " Connection successful!"<<std::endl; 

        return true; 
    }
    catch(std::exception &e){ 
        std::cerr<<"CLIENT attemptConnection ERROR: "<<e.what()<<std::endl; 
        return false; 
    }
    
}


// TO BE COMPLETED BY PHASE 3
bool Client::connectTo(const std::string &endpt, const uint16_t port){
    try{
        
        boost::asio::ip::tcp::resolver resolver(this->client_ctx); 
        auto results = resolver.resolve(endpt, std::to_string(port)); 
        std::string omar = results.begin()->endpoint().address().to_string(); 

        this->client_connection = std::make_shared<Connection>(
            this->client_ctx, tcp::socket(this->client_ctx) ,Owner::CLIENT, this->in_queue, omar
        ); 

        this->client_connection->connectToServer(results); 

        return true; 

    }
    catch(std::exception e){
        std::cout<<"ERROR: "<<e.what()<<std::endl; 
        return false; 
    }
}


bool Client::isConnected(){
    return this->client_connection->isConnected(); 
}

bool Client::disconnect(){
    if(this->client_connection->isConnected()){
        this->client_connection->disconnect(); 
    }

    this->client_ctx.stop(); 

    if(this->ctxThread.joinable()){
        this->ctxThread.join(); 
    }

    return true; 

}


TSQ<OwnedSentMessage>& Client::getInQueue(){
    return this->in_queue; 
}






