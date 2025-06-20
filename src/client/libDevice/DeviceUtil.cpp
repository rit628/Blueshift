#include "DeviceUtil.hpp"
#include <stdexcept>
#ifdef __RPI64__
#include <pigpio.h>
#endif

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

AbstractDevice::AbstractDevice(DEVTYPE dtype, std::unordered_map<std::string, std::string> &port_nums) {
    switch(dtype){
        #define DEVTYPE_BEGIN(name) \
        case DEVTYPE::name: { \
            this->device.emplace<Device<TypeDef::name>>(); \
            this->init(port_nums); \
            break; \
        }
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        default : {
            throw std::invalid_argument("Unknown dtype accessed!"); 
        }
    }
}

void AbstractDevice::processStatesImpl(DynamicMessage& input) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&input](auto& dev) { dev.processStates(input); }
    }, device);
}

void AbstractDevice::processStates(DynamicMessage dmsg) {
    if (!hasInterrupt()) {
        processStatesImpl(dmsg);
        return;
    }
    // wait for interruptors to stop
    {
        std::unique_lock lk(m);
        cv.wait(lk, [this]{ return !watchersPaused; });
    }
    // std::cout << "step 1: aquired lock from manageWatcher after unpausing" << std::endl;

    {
        std::lock_guard lk(m);
        processing = true;
    }
    // std::cout << "step 2: processing to true, notify manageWatcher" << std::endl;
    cv.notify_one();

    {
        std::unique_lock lk(m);
        cv.wait(lk, [this]{ return watchersPaused; });
    }
    // std::cout << "step 5: aquired lock from manageWatcher after pausing" << std::endl;
    
    // tell interruptors to start again
    {
        std::lock_guard lk(m);
        processStatesImpl(dmsg);
        processing = false;
    }
    // std::cout << "step 6: processing to false, notify manageWatcher" << std::endl;
    cv.notify_one();
}

void AbstractDevice::init(std::unordered_map<std::string, std::string> &config) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&config](auto& dev) { dev.init(config); }
    }, device);
}

void AbstractDevice::transmitStates(DynamicMessage &dmsg) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&dmsg](auto& dev) { dev.transmitStates(dmsg); }
    }, device);
}

bool AbstractDevice::hasInterrupt() {
    return std::visit(overloads {
        [](std::monostate&) { return false; },
        [](auto& dev) { return dev.hasInterrupt; }
    }, device);
}

std::vector<Interrupt_Desc>& AbstractDevice::getIdescList() {
    return std::visit(overloads {
        [](std::monostate&) -> std::vector<Interrupt_Desc>& { /* never called */ },
        [](auto& dev) -> std::vector<Interrupt_Desc>& { return dev.Idesc_list; }
    }, device);
}

std::chrono::milliseconds DeviceTimer::getRemainingTime() {
    auto now_time = std::chrono::steady_clock::now(); 
    auto timer_exp = timer.expires_at(); 

    if(now_time > timer_exp){
        auto difference = timer_exp - now_time; 
        return std::chrono::duration_cast<std::chrono::milliseconds>(difference);
    }   
    return std::chrono::milliseconds(0); 
}

void DeviceTimer::timerCallback() {
    this->timer.expires_after(this->period_time); 

    this->timer.async_wait([this](const boost::system::error_code &ec){
        if(!ec){
            this->sendData(); 
            timerCallback(); 
        }
    }); 
}

void DeviceTimer::setPeriod(int new_period) {
    if(this->is_set == false){
        this->is_set = true; 
    }

    auto remTime = getRemainingTime(); 
    this->period_time = std::chrono::milliseconds(new_period); 

    // If new intialization or if the newtime is less than the remaining time
    if(this->period_time < remTime || this->poll_period == -1){

        this->timer.cancel();

        timerCallback(); 
        
        this->poll_period = new_period; 
    }

}

void DeviceTimer::Send(SentMessage &msg) {
    this->conex->send(msg); 
}

float DeviceTimer::calculateStd(std::deque<float> &data) {
    // Calculate the sum first: 
    float sum = 0; 
    float std = 0; 
    int size = data.size(); 

    for(auto& i : data){
        sum += i; 
    }
    float mean = sum/size; 

    for(auto& i : data){
        // Can never be negative as the difference is squared
        std += pow(i - mean, 2); 
    }

    return sqrt(std/size); 
}

void DeviceTimer::calcVolMap() {
    for(auto &pair : this->attr_history){
        this->vol_map[pair.first] = calculateStd(pair.second);
    }
}

void DeviceTimer::sendData() {
    SentMessage smsg; 

    DynamicMessage dmsg; 
    this->device.transmitStates(dmsg); 

    // Extract numerical data about the fields and add to the src: 
    dmsg.getFieldVolatility(this->attr_history, VOLATILITY_LIST_SIZE); 

    if(this->attr_history.size() > 0){
        calcVolMap(); 
        dmsg.createField("__DEV_ATTR_VOLATILITY__", this->vol_map); 
    }

    // Do some kind of data transformation here
    smsg.body = dmsg.Serialize(); 

    smsg.header.ctl_code = this->ctl_code; 
    smsg.header.device_code = this->device_code; 
    smsg.header.timer_id = this->timer_id; 
    smsg.header.prot = Protocol::SEND_STATE; 
    smsg.header.body_size = smsg.body.size(); 
    smsg.header.fromInterrupt = false; 

    Send(smsg); 
}

void DeviceInterruptor::sendMessage() {
    // Configure sent message header: 
    SentMessage sm; 
    sm.header.ctl_code = this->ctl_code; 
    sm.header.device_code = this->device_code; 
    sm.header.prot = Protocol::SEND_STATE; 
    sm.header.fromInterrupt = true; 

    // Change the timer_id specification: 
    sm.header.timer_id = -1;

    // Volatility does not need to be recorded
    sm.header.volatility = 0; 

    DynamicMessage dmsg; 
    this->abs_device.transmitStates(dmsg); 
    sm.body = dmsg.Serialize(); 

    sm.header.body_size = sm.body.size() ; 

    this->client_connection->send(sm); 
}

void DeviceInterruptor::disableWatchers() { 
    for (auto&& [fd, wd, filename] : watchDescriptors) {
        inotify_rm_watch(fd, wd);
    }
}

void DeviceInterruptor::enableWatchers() {
    for (auto& [fd, wd, filename] : watchDescriptors) {
        wd = inotify_add_watch(fd, filename.c_str(), IN_CLOSE_WRITE); 
        if(wd < 0){
            std::cerr<<"Could not add watcher"<<std::endl; 
            close(fd);
        }
    }
}

void DeviceInterruptor::manageWatchers() {
    while (true) {
        auto& m = this->abs_device.m;
        auto& cv = this->abs_device.cv;
        auto& processing = this->abs_device.processing;
        auto& watchersPaused = this->abs_device.watchersPaused;
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&processing] { return processing; });
        }
        // std::cout << "step 3: aquired lock from processStates after init" << std::endl;

        {
            std::lock_guard lk(m);
            disableWatchers();
            watchersPaused = true;
        }
        // std::cout << "step 4: watchersPaused to true, notify processStates" << std::endl;
        cv.notify_one();

        {
            std::unique_lock lk(m);
            cv.wait(lk, [&processing] { return !processing; });
        }
        // std::cout << "step 7: aquired lock from processStates after processing" << std::endl;

        {
            std::lock_guard lk(m);
            enableWatchers();
            watchersPaused = false;
        }
        // std::cout << "step 8: watchersPaused to false, notify processStates" << std::endl;
        cv.notify_one();
    }
}

void DeviceInterruptor::IFileWatcher(std::string fname, std::function<bool()> handler) {
    // Check if the file exists relative to the deviceCore; 
    
    int fd = inotify_init(); 
    if(fd < 0){
        std::cerr<<"Could not make Inotify object"<<std::endl; 
        close(fd); 
    }

    // The IFileWatcher makes the call when the file is modified
    int wd = inotify_add_watch(fd, fname.c_str(), IN_CLOSE_WRITE); 
    if(wd < 0){
        std::cerr<<"Could not perform an inotify event"<<std::endl; 
        close(fd); 
    }

    watchDescriptors.push_back({fd, wd, fname});

    // For now we can bypass the metadata and store data for the filesize and stuff;
    char event_buffer[sizeof(inotify_event) + 256]; 
    while(true){
        std::cout<<"Waiting for event"<<std::endl;
        int read_length = read(fd, event_buffer, sizeof(event_buffer)); 
        struct inotify_event* event = (struct inotify_event*)event_buffer; 
        if (event->mask == IN_IGNORED) {
            std::cout << "drop removed watch event" << std::endl;
            continue;
        }

        bool ret_val = handler(); 
        if(ret_val && !this->abs_device.processing){
            std::cerr<<"Sending message"<<std::endl;
            this->sendMessage();  
        }
    }
}

void DeviceInterruptor::IGpioWatcher(int portNum, std::function<bool(int, int , uint32_t)> interruptHandle) {
    std::pair<bool, std::tuple<int, int, uint32_t>> data; 

    auto popArgs = [](int gpio, int level, unsigned int tick, void* data) -> void{
        auto& [signaler, args] = *(std::pair<bool, std::tuple<int, int, uint32_t>> *)data;
        args = std::make_tuple(gpio, level, tick); 
        signaler = true; 
    }; 

    #ifdef __RPI64__
        gpioSetAlertFuncEx(portNum, popArgs, &data);
    #endif

    while (true) {
        auto& [signaler, args] = data;
        if(signaler){
            bool ret = std::apply(interruptHandle, args); 
            if(ret){
                this->sendMessage();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            signaler = false;
        }
    }
}

void DeviceInterruptor::setupThreads() {
    // Create the threads
    for(auto& idesc : this->abs_device.getIdescList()){
        switch(idesc.src_type){
            case(SrcType::UNIX_FILE) : {
                this->globalWatcherThreads.emplace_back([idesc, this](){
                    this->IFileWatcher(idesc.file_src, std::get<std::function<bool()>>(idesc.interruptHandle)); 
                }); 
                break; 
            }

            case(SrcType::GPIO): {
                this->globalWatcherThreads.emplace_back([&, this](){
                    this->IGpioWatcher(idesc.port_num, std::get<std::function<bool(int, int, uint32_t)>>(idesc.interruptHandle));         
                });
                break; 
            }

            default :{
                std::cout<<"Unimplemented interrupt watcher"<<std::endl; 
            }
        }
    }
    watcherManagerThread = std::thread([this] { this->manageWatchers(); });
}