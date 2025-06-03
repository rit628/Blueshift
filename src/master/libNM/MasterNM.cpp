#include "MasterNM.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include <algorithm>
#include <exception>


MasterNM::MasterNM(std::vector<OBlockDesc> &desc_list, TSQ<DMM> &in_msg, TSQ<DMM> &out_q)
: master_socket(master_ctx), master_acceptor(master_ctx, tcp::endpoint(tcp::v4(), MASTER_PORT)), 
  EMM_in_queue(in_msg), EMM_out_queue(out_q), tickerTable(desc_list)
{
    std::cout<<"Master started!"<<std::endl; 
    writeConfig(desc_list); 

}


void MasterNM::broadcastIntro(){
    try{
        auto bcast = udp::socket(this->master_ctx, udp::endpoint(udp::v4(), 0)); 
        bcast.set_option(boost::asio::socket_base::broadcast(true)); 
        udp::endpoint bcast_endpt(boost::asio::ip::address_v4::broadcast(), BROADCAST_PORT); 
                
        while(this->remConnections > 0){
            for(auto s : this->controller_list){
                if(this->connection_map.find(s) == this->connection_map.end()){
                    std::cout<<"Broadcasting for: "<<s<<std::endl; 
                    bcast.send_to(boost::asio::buffer(s.c_str(), s.size()), bcast_endpt); 
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2500)); 
        }
    }
    catch(std::exception e){
        std::cout<<"MASTER NM ERROR: "<<e.what()<<std::endl; 
    }
}


void MasterNM::writeConfig(std::vector<OBlockDesc> &desc_list){
    std::set<std::string> c_list; 
    std::set<std::string> dev_list; 

    for(auto &oblock : desc_list){
        for(auto &dev : oblock.binded_devices){
            dev_list.insert(dev.device_name); 
            c_list.insert(dev.controller); 
        }
    }

    int i = 0; 
    for(auto &name : c_list){
        this->controller_alias_map[name] = i; 
        this->controller_list.push_back(name); 
        i++; 
    }

    i = 0;
    for(auto &name : dev_list){
        this->device_alias_map[name] = i; 
        this->device_list.push_back(name); 
        i++; 
    } 

    this->remConnections = this->controller_list.size(); 


    // Configure the controller config data once the mappings are made
    for(auto &oblock : desc_list){
        for(auto &dev : oblock.binded_devices){
            // used for debugging 
            this->dd_map[dev.device_name] = dev; 
            
            this->ctl_configs[dev.controller].device_alias.push_back(this->device_alias_map[dev.device_name]); 
            this->ctl_configs[dev.controller].type.push_back(dev.type); 
            this->ctl_configs[dev.controller].srcs.push_back(dev.port_maps); 
            std::cout<<"MASTER - Is Trigger: "<<dev.isTrigger<<std::endl; 
            this->ctl_configs[dev.controller].triggers.push_back(dev.isTrigger); 
            
            dev_list.insert(dev.device_name); 
            c_list.insert(dev.controller);
        }
        
        this->oblock_list.push_back(oblock.name); 
        
    }
}

bool MasterNM::start(){
    try{
        this->bcast_thread = std::thread([this](){this->broadcastIntro();}); 
        // Start the context thread
        listenForConnections(); 
        this->ctx_thread = std::thread([this](){this->master_ctx.run();}); 
        // This is bad (its wasting a lot of CPU cycles)
        this->updateThread = std::thread([this](){this->update();});
     
        if(this->bcast_thread.joinable()){
            this->bcast_thread.join(); 
        }
    
        this->readerThread = std::thread([this](){this->masterRead();}); 

        return true; 

    }
    catch(std::exception e){
        std::cout<<"[SERVER START EXCEPTION] "<<e.what()<<std::endl; 
        return false; 
    }
}

void MasterNM::stop(){
    this->master_ctx.stop(); 
    // Wait on thread by joining with main thread
    if(this->ctx_thread.joinable()){
        this->ctx_thread.join(); 
    }

    this->in_operation = false; 
    if(this->readerThread.joinable()){
        this->readerThread.join(); 
    }

    if(this->updateThread.joinable()){
        this->updateThread.join(); 
    }

    std::cout<<"Server has closed"<<std::endl; 
}

// Async: Waits for client connections
void MasterNM::listenForConnections(){
    this->master_acceptor.async_accept([this]
    (boost::system::error_code ec, tcp::socket socket){

        if(!ec){
        auto endpt = socket.remote_endpoint().address().to_string(); 
        auto newCon = std::make_shared<Connection>(
            this->master_ctx, std::move(socket), Owner::MASTER, this->in_queue, endpt);

        std::cout<<"CONNECTION RECIEVED"<<std::endl;     

        this->temp_vector.push_back(newCon); 
        //this->connection_map["unassigned"] = std::move(newCon);
        newCon->connectToClient(); 

        std::cout<<"MOVED CONNECTION"<<std::endl; 

        // Send Messages to logic: 
        listenForConnections(); 
        }
        else{
            std::cout<<"MASTERNM Connecting Error: "<<ec.message()<<std::endl; 
        }
    }); 

}

// Void message client
void MasterNM::messageClient(const std::string &controller, const SentMessage& sm){
    std::shared_ptr<Connection> client;
    auto test = this->connection_map.find(controller); 
    
    if(test != this->connection_map.end()){
        client = test->second; 
    }
    else{
        std::cerr<<"MASTER NM: Could not find controller of name: " + controller<<std::endl; 
    }

    if(client && client->isConnected()){
        client->send(sm); 
    }
    else{
        onClientDisconnect(controller); 
        client.reset(); 
        this->connection_map.erase(controller); 
    }

}

// Messages all client
void MasterNM::messageAllClients(const SentMessage &sm){
    bool hasNullConnections = false; 
    // Loop through connections and determine of null connections exist
    for(auto i : this->connection_map){
        if(i.first !=  "unassigned"){
            if(i.second && i.second->isConnected()){
                i.second->send(sm); 
            }
            else{
                hasNullConnections = true; 
            }
        }
    }

    // remove connections if necessary
    if(hasNullConnections){
        for (auto it = this->connection_map.begin(); it != this->connection_map.end(); ) {
            if (it->second == nullptr) {
                it = this->connection_map.erase(it); 
            } else {
                ++it;
            }
        }
    }
}


// Reads a state and write it to the queue
void MasterNM::masterRead(){

    while(true){

        // Send the normal message:

        auto new_state = this->EMM_in_queue.read(); 

        std::cout<<"MASTER NM MESSAGE RECEIVED"<<std::endl; 

        SentMessage sm_main;

        std::string cont = new_state.info.controller; 

        sm_main.header.device_code = this->device_alias_map[new_state.info.device]; 
        sm_main.header.ctl_code = this->controller_alias_map[new_state.info.controller]; 
        sm_main.header.prot = Protocol::STATE_CHANGE; 
        sm_main.body = new_state.DM.Serialize(); 
        sm_main.header.body_size = sm_main.body.size(); 

        messageClient(cont, sm_main); 

        // Send the Ticker Update Message (idk maybe)

        std::vector<Timer> timer_list; 
        this->tickerTable.sendTicker(timer_list, cont, this->device_alias_map);
        if(!timer_list.empty()){

            SentMessage sm_update; 
            DynamicMessage dmsg; 

            dmsg.createField("__TICKER_UPDATE__", timer_list); 
            sm_update.body = dmsg.Serialize(); 

            sm_update.header.ctl_code = this->controller_alias_map[new_state.info.controller]; 
            sm_update.header.device_code = this->device_alias_map[new_state.info.device]; 
            sm_update.header.prot = Protocol::TICKER_UPDATE; 
            sm_update.header.body_size = sm_update.body.size(); 
            
            messageClient(cont, sm_update); 
        }
        
    }
}

void MasterNM::update(){
    while(true){
        auto omar = this->in_queue.read();
        std::cout<<"Recieved the message MASTER Network Manager"<<std::endl; 
        this->handleMessage(omar);
    }
}


void MasterNM::handleMessage(OwnedSentMessage &in_msg){
    DynamicMessage dmsg; 
    dmsg.Capture(in_msg.sm.body); 

    switch(in_msg.sm.header.prot){
        case(Protocol::CONFIG_NAME):{

            std::string ctl_name; 
            dmsg.unpack("__CONTROLLER_NAME__", ctl_name);
            auto it = std::find(this->controller_list.begin(), this->controller_list.end(), ctl_name); 

            std::cout<<"REACHED CLIENT CONFIG"<<std::endl; 

            if(it != this->controller_list.end()){
                this->connection_map[ctl_name] = std::move(in_msg.connection);
                this->connection_map[ctl_name]->setName(ctl_name);
                confirmClient(this->connection_map[ctl_name]);  
            } 
            else{
                in_msg.connection->disconnect();
                in_msg.connection.reset(); 
            }

            break; 
        }
        case(Protocol::CONFIG_OK) : {
            this->remConnections--; 
            // Send the initital Ticker Entry: 
            std::cout<<this->controller_list[in_msg.sm.header.ctl_code]<<" has successfully connected!"<<std::endl; 

            break; 
        }
        case(Protocol::CLIENT_ERROR): {
            // For now throw tht g
            std::string error_msg; 
            dmsg.unpack("message", error_msg); 
            std::cout<<error_msg<<std::endl;  
            if(in_msg.sm.header.ec <= ERROR_T::FATAL_ERROR){
                throw BlsExceptionClass("unhandled exception " + error_msg, in_msg.sm.header.ec);
            }
            
            break; 
        }
        case(Protocol::SEND_STATE_INIT) :
        case(Protocol::SEND_STATE) : {
            if(this->remConnections == 0){
                //std::cout<<"SEND STATE DEVICE RECEIVED"<<std::endl; 

                // Get the timer id: 
                TimerID id = in_msg.sm.header.timer_id; 
                DevAlias device_name = this->device_list[in_msg.sm.header.device_code]; 


                bool interrupt = in_msg.sm.header.fromInterrupt; 

                // insert the volatility into the ticker_table: 
                if(!interrupt){
                    // Get the block from the timer_id
                    auto oblock_list = this->tickerTable.getOblocks(id); 

                    std::unordered_map<AttrAlias, float> vol_map; 

                    if(dmsg.hasField("__DEV_ATTR_VOLATILITY__")){
                        dmsg.unpack("__DEV_ATTR_VOLATILITY__", vol_map); 
                        this->tickerTable.updateVolH(device_name, vol_map); 
                    }

                    //std::cout<<"Not Interrupt?"<<std::endl; 
                    for(auto &o_name : oblock_list){
                        DMM new_msg; 
                        new_msg.info.controller = this->controller_list[in_msg.sm.header.ctl_code]; 
                        new_msg.info.device = device_name; 
                        new_msg.info.oblock = o_name; 
                        new_msg.DM = dmsg; 
                        new_msg.isInterrupt = false; 
                        new_msg.protocol = PROTOCOLS::SENDSTATES; 
                        //std::cout<<"Write to queue"<<std::endl; 
                        
                        this->EMM_out_queue.write(new_msg); 
                    }
                }
                else{
                    DMM new_msg; 
                    new_msg.info.controller = this->controller_list[in_msg.sm.header.ctl_code]; 
                    new_msg.info.device = device_name;
                    // Oblock not used for interrupt based devices 
                    new_msg.info.oblock = ""; 
                    new_msg.DM = dmsg; 
                    new_msg.isInterrupt = true;
                    new_msg.protocol = PROTOCOLS::SENDSTATES;
                    //std::cout<<"Write to queue"<<std::endl; 

                    this->EMM_out_queue.write(new_msg); 
                }

            }
            else{
                std::cerr<<"Cannot process state until all required devices are configured"<<std::endl; 
            }
            break;
        }
        case(Protocol::CALLBACK): {

            std::string device_name = this->device_list[in_msg.sm.header.device_code];

            DMM new_msg; 

            new_msg.info.controller = this->controller_list[in_msg.sm.header.ctl_code]; 
            new_msg.info.device = device_name;         
            new_msg.protocol = PROTOCOLS::CALLBACKRECIEVED; 
            new_msg.DM = dmsg; 
            new_msg.isInterrupt = in_msg.sm.header.fromInterrupt;
            this->EMM_out_queue.write(new_msg);

            break; 
        }
        default:{
            std::cerr<<"MASTER NM unknown handle"<<std::endl; 
        }
    }
}


// Creates the config message and send it to the client (assuming the target is found)
bool MasterNM::confirmClient(std::shared_ptr<Connection> &con_obj){
    std::string c_name = con_obj->getName();
    SentMessage dev_sm; 
    dev_sm.header.prot = Protocol::CONFIG_INFO; 
    dev_sm.header.ctl_code = this->controller_alias_map[c_name]; 
    // Device code doesnt matter 
    dev_sm.header.device_code = 0; 

    // Copy the data: 
    DynamicMessage dmsg; 

    // Sends the configuration info for all the devices
    dmsg.createField("__DEV_ALIAS__" ,this->ctl_configs[c_name].device_alias); 
    dmsg.createField("__DEV_TYPES__" ,this->ctl_configs[c_name].type);
    dmsg.createField("__DEV_PORTS__" ,this->ctl_configs[c_name].srcs);  
    dmsg.createField("__DEV_INIT__", this->ctl_configs[c_name].triggers);

    dev_sm.body = dmsg.Serialize(); 
    dev_sm.header.body_size = dev_sm.body.size(); 

    // send ticker data
    this->sendInitialTicker(con_obj); 

    con_obj->send(dev_sm); 



    return true; 
}

// Makes the beginning call
void MasterNM::makeBeginCall(){
    std::cout<<"Beginning the Sending process!"<<std::endl; 
    // send out client start (might not need this due to the initialization of the sending process)
    SentMessage ok_start; 
    ok_start.header.prot = Protocol::BEGIN; 
    ok_start.header.body_size = 0; 
    this->messageAllClients(ok_start);
}

// Sends the intital ticker data
void MasterNM::sendInitialTicker(std::shared_ptr<Connection> &con_obj){
    auto c_name =  con_obj->getName();
    SentMessage ticker_sm; 
    ticker_sm.header.prot = Protocol::TICKER_INITIAL; 
    ticker_sm.header.ctl_code = this->controller_alias_map[c_name]; 
    ticker_sm.header.device_code = 0; 

    DynamicMessage tick_dmsg; 
    std::vector<Timer> initialTimers; 
    this->tickerTable.sendInitial(initialTimers, c_name, this->device_alias_map); 
    if(initialTimers.size() > 0){
        tick_dmsg.createField("__TICKER_DATA__", initialTimers); 
        ticker_sm.body = tick_dmsg.Serialize(); 
        ticker_sm.header.body_size = ticker_sm.body.size(); 
    }
    else{
        ticker_sm.header.body_size = 0; 
        ticker_sm.body = {}; 
    }
    
    con_obj->send(ticker_sm); 

   

    std::cout<<"Sent inital ticker"<<std::endl; 
}


// Client disconnect
void MasterNM::onClientDisconnect(const std::string &controller){

    std::cout<<"Controller " + controller<<" has disconnected from the system!"<<std::endl; 
}

MasterNM::~MasterNM(){
    this->stop(); 
}





