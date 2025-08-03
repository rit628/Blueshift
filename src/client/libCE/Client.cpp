#include "Client.hpp"
#include "include/Common.hpp"
#include "libDevice/DeviceUtil.hpp"
#include "libDevice/include/ADC.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include <functional>
#include <exception>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <sys/socket.h>

Client::Client(std::string c_name): bc_socket(client_ctx, udp::endpoint(udp::v4(), BROADCAST_PORT)), client_socket(client_ctx){
    this->client_name = c_name; 
    std::cout<<"Client Created: " << c_name <<std::endl; 
}

void Client::start(){
    std::cout<<"Starting Client"<<std::endl; 
    this->broadcastListen(); 
}


void Client::sendMessage(uint16_t deviceCode, Protocol type, bool fromInt = false, oblock_int oint = 0, bool write_self = false){
    // Write code for a callback
    SentMessage sm; 
    DynamicMessage dmsg; 


    switch(type){
        case(Protocol::OWNER_GRANT):{
            sm.header.oblock_id = oint; 
            sm.header.prot = Protocol::OWNER_GRANT; 
            sm.header.device_code = deviceCode; 
            break; 
        }
        case(Protocol::OWNER_CONFIRM_OK) : {
            sm.header.oblock_id = oint; 
            sm.header.prot = Protocol::OWNER_CONFIRM_OK; 
            sm.header.device_code = deviceCode;
            break; 
        }
        default: {
            // Get the latest state from the dmsg
            try{
                std::cout<<"ACCESING device: "<<deviceCode<<std::endl;
                this->deviceList.at(deviceCode).device.transmitStates(dmsg); 
            }
            catch(BlsExceptionClass& bec){
                this->genBlsException->SendGenericException(bec.what(), bec.type());
                return; 
            }

            // make message
            sm.body = dmsg.Serialize(); 
            sm.header.ctl_code = this->controller_alias;
            sm.header.prot = type; 
            sm.header.device_code = deviceCode; 
            sm.header.timer_id = -1; 
            sm.header.volatility = -1; 
            sm.header.body_size = sm.body.size(); 
            sm.header.fromInterrupt = fromInt; 
            
            break; 
        }
    }

    if(write_self){
        OwnedSentMessage osm; 
        osm.sm = sm; 
        osm.connection = nullptr; 
        this->in_queue.write(osm); 
    }
    else{
        std::cout<<"State for device: "<<deviceCode<<std::endl; 
        this->client_connection->send(sm); 
    }
}

void Client::listener(std::stop_token stoken){
    while(!stoken.stop_requested()){

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
            uint8_t controller_alias = inMsg.header.ctl_code; 

            dmsg.unpack("__DEV_ALIAS__", device_alias);
            dmsg.unpack("__DEV_TYPES__", device_types); 
            dmsg.unpack("__DEV_PORTS__", srcs); 

            this->controller_alias = controller_alias; 

            // Check that all vectors are of equal size and more than 0 devices are configured: 
            int size = device_alias.size(); 
            bool b = device_types.size() == size; 
            bool c = srcs.size() == size; 
            

            if(!(b && c)){
                throw std::invalid_argument("Config vectors of different sizes what!"); 
            }

            // Try for existance (replace with function to find connected ADCs when more are added)
        
            this->adc = std::make_shared<ADS7830>();

            for(int i = 0; i < size; i++){
                try{      
                    std::cout<<"Emplacing: "<<device_alias[i]<<std::endl;
                    deviceList.try_emplace(device_alias[i], device_types[i], srcs[i], this->adc);
                    std::cout<<"Emplace device: "<<device_alias[i]<<std::endl;
                }
                catch(BlsExceptionClass& bec){
                    std::cout<<"Failed to emplace device"<<std::endl;
                    this->genBlsException->SendGenericException(bec.what(), bec.type()); 
                }
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

                // Check if the writer is the valid owner of the device (if applies)
                auto& pendingReq = this->deviceList.at(dev_index).pendingRequests; 
                if(pendingReq.currOwned){
                    if((inMsg.header.ctl_code != pendingReq.owner.first) || (inMsg.header.oblock_id != pendingReq.owner.second)){
                        std::cout<<"OWNERSHIP WARNING: "<<std::endl; 
                        std::cout<<"CTL: "<<inMsg.header.ctl_code<<std::endl; 
                        std::cout<<"OBLOCK: "<<inMsg.header.device_code<<std::endl;
                        std::cout<<"Tried to access a preowned device"<<std::endl; 
                        continue; 
                    }
                    std::cout<<"Valid owner"<<std::endl; 
                }


                // Trying out the thread pool: 
                boost::asio::post(this->client_ctx,  [dev_index, dmsg = std::move(dmsg), this](){
                    try{   
                        this->deviceList.at(dev_index).device.processStates(dmsg);
                        this->sendMessage(dev_index, Protocol::CALLBACK, false);
                    }
                    catch(std::exception(e)){
                        std::cout<<"Failure to change the state detected"<<std::endl; 
                        std::cout<<e.what()<<std::endl; 
                    }
                }); 
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
                auto& device = this->deviceList.at(timer.device_num).device; 
            
                if(!device.hasInterrupt()) {

                    if(!device.isActuator){
                        std::cout<<"build timer with period: "<<timer.period<<std::endl;
                        this->client_ticker.try_emplace(timer.id, this->client_ctx, device, this->client_connection, this->controller_alias, timer.device_num, timer.id);
                        this->client_ticker.at(timer.id).setPeriod(timer.period);
                    }
                    std::cout<<"Setting the period"<<std::endl;
                    sendMessage(timer.device_num, Protocol::SEND_STATE_INIT, true); 
                    std::cout<<"SENT MESSAGE"<<std::endl;
                }
            }

             // populate the Device interruptors; 
            for(auto&& [dev_id, dev] : this->deviceList) {
                auto& device = dev.device;
                if(device.hasInterrupt()){
                    // Organizes the device interrupts
                    std::cout<<"Interrupt created!"<<std::endl;
                    this->interruptors.emplace_back(device, this->client_connection, this->controller_alias, dev_id);
                    std::cout<<"Sending Initial State"<<std::endl; 
                    sendMessage(dev_id, Protocol::SEND_STATE_INIT, true); 
                }   
            }
            for (auto&& interruptor : this->interruptors) {
                interruptor.setupThreads();
            }

            #ifdef SDL_ENABLED
            // SDL_RunOnMainThread([](void*) -> void {
            //     auto window = SDL_GL_GetCurrentWindow();
            //     if (SDL_GetHintBoolean("SDL_HINT_INPUT_WINDOW_REQUIRED", false)) {
            //         SDL_ShowWindow(window);
            //     }
            //     else {
            //         SDL_Event event;
            //         event.type = SDL_EVENT_USER;
            //         SDL_PushEvent(&event);
            //     }
            // }, nullptr, true);
            #endif

            // Start the threads
            int num_threads = this->deviceList.size();
            for(int i = 0; i < num_threads ; i++){
                this->threadPool.emplace_back([this]{this->client_ctx.run();});
            }

        }
        else if(ptype == Protocol::CONNECTION_LOST){
            std::cout<<"Connection lost detected by Client"<<std::endl; 
            this->disconnect(); 
            this->adc->close();
        }
        else if(ptype == Protocol::OWNER_CANDIDATE_REQUEST){
            dev_int targDevice = inMsg.header.device_code;

            ClientSideReq csReq; 
            csReq.ctl = inMsg.header.ctl_code; 
            csReq.targetDevice = targDevice; 
            csReq.requestorOblock = inMsg.header.oblock_id;  
            csReq.priority = inMsg.header.oblock_priority; 
            std::cout<<"Adding request for obloc: "<<csReq.requestorOblock<<" with priority "<<csReq.priority<<std::endl; 
            this->deviceList.at(csReq.targetDevice).pendingRequests.getQueue().push(csReq);
        }
        else if(ptype == Protocol::OWNER_CANDIDATE_REQUEST_CONCLUDE){
            std::cout<<"CLIENT SIDE RECEIVED OWNER CANDIDATE REQUEST CONCLUDE"<<std::endl; 
            // Protocol message concludes that all requests have been made in response to a trigger event ()
            // TODO: add support for decentralization
            
            dev_int targDevice = inMsg.header.device_code;
            if(!this->deviceList.at(targDevice).pendingRequests.currOwned){
                    // Add the code to send the device grant here: 
                    auto king = this->deviceList.at(targDevice).pendingRequests.getQueue().top(); 
                    sendMessage(king.targetDevice, Protocol::OWNER_GRANT, false, king.requestorOblock); 
                    this->deviceList.at(targDevice).pendingRequests.currOwned = true; 
            }
        }

        // Confirms the owner (all non owner attempts to access a device are blocked)
        else if(ptype == Protocol::OWNER_CONFIRM){
            std::cout<<"Received confirmation"<<std::endl; 
            dev_int dev_id= inMsg.header.device_code; 
            auto& devicePending = this->deviceList.at(dev_id).pendingRequests; 
            
            // Check if the top value matches the confirmation: 
            auto& pendingSet = devicePending.getQueue(); 
            auto loadedProcess = pendingSet.top(); 
            if((loadedProcess.ctl == inMsg.header.ctl_code) && (loadedProcess.requestorOblock == inMsg.header.oblock_id)){
                sendMessage(dev_id, Protocol::OWNER_CONFIRM_OK, false, inMsg.header.oblock_id);
                devicePending.currOwned = true;
                auto& devPair = this->deviceList.at(dev_id).pendingRequests.owner; 
                devPair = {inMsg.header.ctl_code, inMsg.header.oblock_id}; 
                std::cout<<"Confrm Before Erasure"<<std::endl; 
                pendingSet.pop(); 
                std::cout<<"Confirm After Erasure"<<std::endl; 
            }
            else{
                std::cerr<<"PROTOCOL ERROR: INVALID OWNER CONFIRM FOR PROCESS THAT IS NOT A PRIME CANDIDATE"<<std::endl; 
                std::cout<<"ERROR CTL_CODE: "<<inMsg.header.ctl_code<<" ERROR OBLOCK ID "<<inMsg.header.oblock_id<<std::endl; 
                // Send a grant to the new oblock on top instead
                sendMessage(loadedProcess.targetDevice, Protocol::OWNER_GRANT, false, loadedProcess.requestorOblock);

            }         
        }
        else if(ptype == Protocol::OWNER_RELEASE){
            std::cout<<"Received device release for device"<<std::endl; 
            auto& devPendStruct = this->deviceList.at(inMsg.header.device_code).pendingRequests; 

            std::cout<<"Size at release "<<devPendStruct.getQueue().size()<<std::endl; 
        
            if(!devPendStruct.getQueue().empty()){
                auto& scheduleOrder = devPendStruct.getQueue();
                auto nextProcess = scheduleOrder.top();
                oblock_int o_id = nextProcess.requestorOblock;
                sendMessage(inMsg.header.device_code, Protocol::OWNER_GRANT, false, o_id);
            }
            else{
                devPendStruct.currOwned = false; 
            }
        }
        else{
            std::cout<<"Unknown protocol message!"<<std::endl; 
        }
    }
    
}

void Client::broadcastListen(){
    while(true){
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
            this->client_connection->setName(this->client_name); 
            this->genBlsException = std::make_unique<GenericBlsException>(
                this->client_connection, Owner::CLIENT
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

        this->listenerThread = std::jthread(std::bind(&Client::listener, std::ref(*this), std::placeholders::_1));
        this->ctxThread = std::jthread([this](){this->client_ctx.run();});   
        

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


bool Client::isConnected(){
    return this->client_connection->isConnected(); 
}

bool Client::disconnect(){
    // Closes the socket
    if(this->client_connection->isConnected()){
        this->client_connection->disconnect(); 
    }   

    std::cout<<"Socket Closed"<<std::endl; 

    // Kills the device interruptor and timer threads
    interruptors.clear();

    std::cout<<"Interrupt threads killed"<<std::endl; 

    client_ticker.clear();

    std::cout<<"Timers listeners killed"<<std::endl; 
    
    this->client_ctx.stop(); 
    for(auto &t : this->threadPool){
        t.join();
    }
    this->listenerThread.request_stop();


    std::cout<<"Context and client listener killed"<<std::endl;

    this->deviceList.clear();
    this->start_timers.clear();
    
    std::cout << "Client device data reset" << std::endl;

    return true; 
}


TSQ<OwnedSentMessage>& Client::getInQueue(){
    return this->in_queue; 
}






