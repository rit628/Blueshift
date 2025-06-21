#include "DeviceUtil.hpp"
#include <stdexcept>
#include <stop_token>
#include <variant>
#ifdef __RPI64__
#include <pigpio.h>
#endif

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

DeviceHandle::DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string> &config, uint16_t sendInterrupt) {
    switch(dtype){
        #define DEVTYPE_BEGIN(name) \
        case TYPE::name: { \
            this->device.emplace<Device::name>(); \
            this->init(config); \
            this->isTrigger = sendInterrupt; \
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

void DeviceHandle::processStatesImpl(DynamicMessage& input) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&input](auto& dev) { dev.processStates(input); }
    }, device);
}

void DeviceHandle::processStates(DynamicMessage dmsg) {
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

void DeviceHandle::init(std::unordered_map<std::string, std::string> &config) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&config](auto& dev) { dev.init(config); }
    }, device);
}

void DeviceHandle::transmitStates(DynamicMessage &dmsg) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&dmsg](auto& dev) { dev.transmitStates(dmsg); }
    }, device);
}

bool DeviceHandle::hasInterrupt() {
    return std::visit(overloads {
        [](std::monostate&) { return false; },
        [](auto& dev) { return dev.hasInterrupt; }
    }, device);
}

std::vector<Interrupt_Desc>& DeviceHandle::getIdescList() {
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

DeviceTimer::~DeviceTimer() {
    this->timer.cancel();
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
    this->device.transmitStates(dmsg); 
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

void DeviceInterruptor::manageWatchers(std::stop_token stoken) {
    auto& m = this->device.m;
    auto& cv = this->device.cv;
    auto& processing = this->device.processing;
    auto& watchersPaused = this->device.watchersPaused;
    while (!stoken.stop_requested()) {
        // wait for device to receive a new message to process or for device reset
        {
            std::unique_lock lk(m);
            if (!cv.wait(lk, stoken, [&processing] { return processing; })) {
                break; // break early to avoid unnecessary lock aquisition overhead
            }
        }

        // disable watchers before message processing
        {
            std::lock_guard lk(m);
            disableWatchers();
            watchersPaused = true;
        }
        cv.notify_one();

        // wait for message to process
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&processing] { return !processing; });
        }

        // re-enable watchers
        {
            std::lock_guard lk(m);
            enableWatchers();
            watchersPaused = false;
        }
        cv.notify_one();
    }
    disableWatchers();
}

void DeviceInterruptor::IFileWatcher(std::stop_token stoken, std::string fname, std::function<bool()> handler){
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
    while(!stoken.stop_requested()){
        std::cout<<"Waiting for event"<<std::endl;
        int read_length = read(fd, event_buffer, sizeof(event_buffer)); 

        struct inotify_event* event = (struct inotify_event*)event_buffer; 
        if (event->mask == IN_IGNORED) {
            std::cout << "drop removed watch event" << std::endl;
            continue;
        }
        bool ret_val = handler(); 
        if(ret_val && !this->device.processing){
            std::cerr<<"Sending message"<<std::endl;
            this->sendMessage();  
        }
    }
}

void DeviceInterruptor::IGpioWatcher(std::stop_token stoken, int portNum, std::function<bool(int, int , uint32_t)> interruptHandle) {
    std::pair<bool, std::tuple<int, int, uint32_t>> data; 

    auto popArgs = [](int gpio, int level, unsigned int tick, void* data) -> void{
        auto& [signaler, args] = *(std::pair<bool, std::tuple<int, int, uint32_t>> *)data;
        args = std::make_tuple(gpio, level, tick); 
        signaler = true; 
    }; 

    #ifdef __RPI64__
        gpioSetAlertFuncEx(portNum, popArgs, &data);
    #endif

    while (!stoken.stop_requested()) {
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

DeviceInterruptor::~DeviceInterruptor() {
    for (auto&& watcher : globalWatcherThreads) {
        watcher.request_stop();
    }
    watcherManagerThread.request_stop();
}

void DeviceInterruptor::setupThreads() {
    // Create the threads
    auto iFileWatcher = std::bind(&DeviceInterruptor::IFileWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    auto iGpioWatcher = std::bind(&DeviceInterruptor::IGpioWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    auto manageWatchers = std::bind(&DeviceInterruptor::manageWatchers, std::ref(*this), std::placeholders::_1);
    for(auto& idesc : this->device.getIdescList()){
        switch(idesc.src_type){
            case(SrcType::UNIX_FILE) : {
                this->globalWatcherThreads.emplace_back(iFileWatcher, idesc.file_src, std::get<std::function<bool()>>(idesc.interruptHandle)); 
                break; 
            }

            case(SrcType::GPIO): {
                this->globalWatcherThreads.emplace_back(iGpioWatcher, idesc.port_num, std::get<std::function<bool(int, int, uint32_t)>>(idesc.interruptHandle));
                break; 
            }

            default :{
                std::cout<<"Unimplemented interrupt watcher"<<std::endl; 
            }
        }
    }
    watcherManagerThread = std::jthread(manageWatchers);
}