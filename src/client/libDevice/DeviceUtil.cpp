#include "DeviceUtil.hpp"
#include "DeviceCore.hpp"
#include "include/ADC.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <sys/inotify.h>
#include <tuple>
#include <utility>
#include <variant>
#include <boost/range/adaptor/map.hpp>
#ifdef __RPI64__
#include <pigpio.h>
#endif

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

DeviceHandle::DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string> &config, std::shared_ptr<ADS7830> usingADC) 
{
    switch(dtype){
        #define DEVTYPE_BEGIN(name) \
        case TYPE::name: { \
            this->device.emplace<Device::name>(); \
            if (config.contains("cooldown")) { \
                cooldown = std::stoul(config.at("cooldown")); \
            } \
            this->init(config, usingADC); \
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
    // notify watchers and timers that message processing is about to begin
    // std::cout << "step 1: waiting for processing to end" << std::endl;
    {
        std::unique_lock lk(m);
        cv.wait(lk, [this]{ return !processing; }); // wait for previous message to process completely
        processing = true;
        // std::cout << "step 2: notifying watchers and timers to pause" << std::endl;
    }
    cv.notify_all();

    // wait until watchers and timers are paused then process message
    {
        std::unique_lock lk(m);
        cv.wait(lk, [this]{ return watchersPaused && timersPaused; });
        processStatesImpl(dmsg);
        processing = false;
        // std::cout << "step 4: processed states, notify watchers and timers to re-enable" << std::endl;
    }
    cv.notify_all(); // notify all watchers and timers to re-enable
}

void DeviceHandle::init(std::unordered_map<std::string, std::string> &config, std::shared_ptr<ADS7830> adc) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&config, adc, this](auto& dev) { 
            std::cout<<"Calling init"<<std::endl; 
            dev.setADC(adc);
            dev.init(config);
            this->isActuator = dev.getActuator();}

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

std::vector<InterruptDescriptor>& DeviceHandle::getIdescList() {
    return std::visit(overloads {
        [](std::monostate&) -> std::vector<InterruptDescriptor>& { throw std::runtime_error("Attempt to access Idesc List for null device."); },
        [](auto& dev) -> std::vector<InterruptDescriptor>& { return dev.Idesc_list; }
    }, device);
}

DevicePoller::DeviceTimer::DeviceTimer(uint16_t id, int period, boost::asio::io_context& ctx, DevicePoller& manager)
                                     : id(id), poll_period(period), timer(ctx), manager(manager) {}

std::chrono::milliseconds DevicePoller::DeviceTimer::getRemainingTime() {
    auto now_time = std::chrono::steady_clock::now(); 
    auto timer_exp = timer.expires_at();

    if(now_time > timer_exp){
        auto difference = timer_exp - now_time; 
        return std::chrono::duration_cast<std::chrono::milliseconds>(difference);
    }   
    return std::chrono::milliseconds(0); 
}

float DevicePoller::DeviceTimer::calculateStd(std::deque<float> &data) {
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

void DevicePoller::DeviceTimer::calcVolMap() {
    for(auto &pair : this->attr_history){
        this->vol_map[pair.first] = calculateStd(pair.second);
    }
}

void DevicePoller::DeviceTimer::timerCallback() {
    this->timer.expires_after(this->period_time); 

    this->timer.async_wait([this](const boost::system::error_code &ec){
        if(!ec){
            manager.sendMessage(*this); 
            timerCallback(); 
        }
    }); 
}

void DevicePoller::DeviceTimer::setPeriod(int newPeriod) {
    auto remTime = getRemainingTime();
    this->period_time = std::chrono::milliseconds(newPeriod);

    // If new intialization or if the newtime is less than the remaining time
    if(this->period_time < remTime || this->poll_period == -1){

        this->timer.cancel();

        timerCallback();
        
        this->poll_period = newPeriod;
    }

}

void DevicePoller::DeviceTimer::start() {
    this->period_time = std::chrono::milliseconds(this->poll_period);
    this->timer.cancel();
    timerCallback();
}

void DevicePoller::DeviceTimer::pause() {
    this->period_time = getRemainingTime();
    this->timer.cancel();
}

void DevicePoller::DeviceTimer::resume() {
    timerCallback();
    this->period_time = std::chrono::milliseconds(this->poll_period);
}

DevicePoller::DevicePoller(boost::asio::io_context &in_ctx, DeviceHandle& device, std::shared_ptr<Connection> cc, int ctl, int dev)
                         : device(device), conex(cc), ctx(in_ctx), ctl_code(ctl), device_code(dev) {}

DevicePoller::DevicePoller(DevicePoller&& other)
                         : device(other.device)
                         , conex(other.conex)
                         , ctx(other.ctx)
                         , ctl_code(other.ctl_code)
                         , device_code(other.device_code)

{
    for (auto&& [id, timer] : other.timers) {
        createTimer(id, timer.poll_period); // recreate timers other's timers to include reference to this
    }
    if (other.timerManagerThread.joinable()) { // restart timers
        startTimers();
    }
}

DevicePoller::~DevicePoller() {
    if (timerManagerThread.joinable()) {
        timerManagerThread.request_stop();
        timerManagerThread.join();
    }
    this->timers.clear();
}

void DevicePoller::pauseTimers() {
    for (auto&& [_, timer] : timers) {
        timer.pause();
    }
}

void DevicePoller::resumeTimers() {
    for (auto&& [_, timer] : timers) {
        timer.resume();
    }
}

void DevicePoller::manageTimers(std::stop_token stoken) {
    auto& m = this->device.m;
    auto& cv = this->device.cv;
    auto& processing = this->device.processing;
    auto& timersPaused = this->device.timersPaused;
    timersPaused = false;
    while (!stoken.stop_requested()) {
        // wait for device to receive a new message to process then disable timers
        {
            std::shared_lock lk(m);
            if (!cv.wait(lk, stoken, [&processing] { return processing; })) {
                break; // break early to avoid unnecessary lock aquisition overhead
            }
            pauseTimers();
            timersPaused = true;
            // std::cout << "step 3: paused timers, notifying processing to begin" << std::endl;
        }
        cv.notify_all();

        // wait for message to process then re-enable timers
        {
            std::shared_lock lk(m);
            if (!cv.wait(lk, stoken, [&processing] { return !processing; })) {
                break; // break early to avoid unnecessary re-enabling
            }
            resumeTimers();
            timersPaused = false;
            // std::cout << "step 5: timers re-enabled" << std::endl;
        }
    }
    pauseTimers();
}

void DevicePoller::sendMessage(DeviceTimer& timer) {
    SentMessage smsg; 

    DynamicMessage dmsg; 
    this->device.transmitStates(dmsg); 

    // Extract numerical data out the fields and add to the src: 
    dmsg.getFieldVolatility(timer.attr_history, VOLATILITY_LIST_SIZE); 

    if(timer.attr_history.size() > 0){
        timer.calcVolMap(); 
        dmsg.createField("__DEV_ATTR_VOLATILITY__", timer.vol_map); 
    }

    // Do some kind of data transformation here
    smsg.body = dmsg.Serialize(); 

    smsg.header.ctl_code = this->ctl_code; 
    smsg.header.device_code = this->device_code; 
    smsg.header.timer_id = timer.id; 
    smsg.header.prot = Protocol::SEND_STATE; 
    smsg.header.body_size = smsg.body.size(); 
    smsg.header.fromInterrupt = false; 

    this->conex->send(smsg);
}

void DevicePoller::setPeriod(uint16_t timerId, int newPeriod) {
    newPeriod = (newPeriod < 0) ? 1000 : newPeriod; // change all negative periods to 1000ms
    timers.at(timerId).setPeriod(newPeriod);
}

void DevicePoller::createTimer(uint16_t timerId, int period) {
    period = (period < 0) ? 1000 : period; // change all negative periods to 1000ms
    timers.try_emplace(timerId, timerId, period, ctx, *this);
}

std::vector<uint16_t> DevicePoller::getTimerIds() {
    auto ids = boost::adaptors::keys(timers);
    return {ids.begin(), ids.end()};
}

void DevicePoller::startTimers() {
    timerManagerThread = std::jthread(std::bind(&DevicePoller::manageTimers, std::ref(*this), std::placeholders::_1));
    for (auto&& [_, timer] : timers) {
        timer.start();
    }
}

DeviceInterruptor::DeviceInterruptor(boost::asio::io_context &in_ctx, DeviceHandle& targDev, std::shared_ptr<Connection> conex, int ctl, int dd)
                                    : device(targDev), client_connection(conex), cooldownTimer(in_ctx), ctl_code(ctl), device_code(dd) {}

DeviceInterruptor::DeviceInterruptor(DeviceInterruptor&& other)
                                    : device(other.device)
                                    , client_connection(other.client_connection)
                                    , cooldownTimer(std::move(other.cooldownTimer))
                                    , ctl_code(other.ctl_code)
                                    , device_code(other.device_code)
{
    running = other.watcherManagerThread.joinable();
    if (running) { // restart threads
        this->setupThreads();
    }
}

DeviceInterruptor::~DeviceInterruptor() {
    this->stopThreads();
}

void DeviceInterruptor::restartCooldown() {
    if (device.cooldown == 0) return;
    cooldownTimer.cancel();
    cooldownTimer.expires_after(std::chrono::milliseconds(device.cooldown));
}

bool DeviceInterruptor::inCooldown() {
    if (device.cooldown == 0) return false;
    auto currentTime = std::chrono::steady_clock::now(); 
    auto cooldownTime = cooldownTimer.expires_at(); 
    return currentTime < cooldownTime;
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
    for (auto&& descriptor : watchDescriptors) {
        std::visit(overloads {
            [](FileWatchDescriptor& desc) {
                auto&& [fd, wd, filename] = desc;
                inotify_rm_watch(fd, wd);
            },
            [](GpioWatchDescriptor& desc) {
                #ifdef __RPI64__
                auto&& [port, callback, callbackData] = desc;
                gpioSetAlertFuncEx(port, NULL, callbackData);
                #endif
            },
            #ifdef SDL_ENABLED
            [](SdlWatchDescriptor& desc) {
                auto&& [callback, callbackData] = desc;
                SDL_RemoveEventWatch(callback, callbackData);
            }
            #endif
        }, descriptor);
    }
}

void DeviceInterruptor::enableWatchers() {
    for (auto&& descriptor : watchDescriptors) {
        std::visit(overloads {
            [](FileWatchDescriptor& desc) {
                auto&& [fd, wd, filename] = desc;
                wd = inotify_add_watch(fd, filename.c_str(), IN_CLOSE_WRITE); 
                if(wd < 0){
                    std::cerr<<"Could not add watcher"<<std::endl; 
                    close(fd);
                }
            },
            [](GpioWatchDescriptor& desc) {
                #ifdef __RPI64__
                auto&& [port, callback, callbackData] = desc;
                gpioSetAlertFuncEx(port, callback, callbackData);
                #endif
            },
            #ifdef SDL_ENABLED
            [](SdlWatchDescriptor& desc) {
                auto&& [callback, callbackData] = desc;
                SDL_AddEventWatch(callback, callbackData);
            }
            #endif
        }, descriptor);
    }
}

void DeviceInterruptor::manageWatchers(std::stop_token stoken) {
    auto& m = this->device.m;
    auto& cv = this->device.cv;
    auto& processing = this->device.processing;
    auto& watchersPaused = this->device.watchersPaused;
    watchersPaused = false;
    while (!stoken.stop_requested()) {
        // wait for device to receive a new message to process then disable watchers
        {
            std::shared_lock lk(m);
            if (!cv.wait(lk, stoken, [&processing] { return processing; })) {
                break; // break early to avoid unnecessary lock aquisition overhead
            }
            disableWatchers();
            watchersPaused = true;
            // std::cout << "step 3: paused watchers, notifying processing to begin" << std::endl;
        }
        cv.notify_all();

        // wait for message to process then re-enable watchers
        {
            std::shared_lock lk(m);
            if (!cv.wait(lk, stoken, [&processing] { return !processing; })) {
                break; // break early to avoid unnecessary re-enabling
            }
            enableWatchers();
            watchersPaused = false;
            // std::cout << "step 5: watchers re-enabled" << std::endl;
        }
    }
    disableWatchers();
    running = false;
    running.notify_all();
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
    watchDescriptors.push_back(FileWatchDescriptor{fd, wd, fname});

    // For now we can bypass the metadata and store data for the filesize and stuff;
    char event_buffer[sizeof(inotify_event) + 256]; 
    while(!stoken.stop_requested()){
        //std::cout<<"Waiting for event"<<std::endl;
        int read_length = read(fd, event_buffer, sizeof(event_buffer)); 

        auto* event = reinterpret_cast<struct inotify_event*>(event_buffer);
        if (event->mask == IN_IGNORED) {
            //std::cout << "drop removed watch event" << std::endl;
            continue;
        }
         
        if(handler() && !inCooldown()){
            std::cerr<<"Sending message"<<std::endl;
            restartCooldown();
            this->sendMessage();  
        }
    }
    running.wait(true); // terminate thread once watcher manager has shutdown
}

void DeviceInterruptor::IGpioWatcher(std::stop_token stoken, int portNum, std::function<bool(int, int , uint32_t)> handler) {
    condition_variable_any cv;
    std::mutex m;
    bool handlerSignal = false;
    using callback_data = std::tuple<condition_variable_any&, std::mutex&, bool&, decltype(handler)&>;
    callback_data callbackData = {cv, m, handlerSignal, handler};
    auto callback = [](int gpio, int level, unsigned int tick, void* callbackData) -> void {
        auto& [cv, m, handlerSignal, handler] = *reinterpret_cast<callback_data*>(callbackData);
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&handlerSignal](){ return !handlerSignal; });
            handlerSignal = handler(gpio, level, tick);
        }
        cv.notify_all();
    };
    watchDescriptors.push_back(GpioWatchDescriptor{portNum, callback, &callbackData});

    #ifdef __RPI64__
    gpioSetAlertFuncEx(portNum, callback, &callbackData);
    #endif

    while (!stoken.stop_requested()) {
        std::unique_lock lk(m);
        if (cv.wait(lk, stoken, [this, &handlerSignal] { return handlerSignal && !inCooldown(); })) {
            restartCooldown();
            this->sendMessage();
            handlerSignal = false;
        }
        cv.notify_one();
    }
    running.wait(true); // terminate thread once watcher manager has shutdown
}

#ifdef SDL_ENABLED
void DeviceInterruptor::ISdlWatcher(std::stop_token stoken, std::function<bool(SDL_Event*)> handler) {
    condition_variable_any cv;
    std::mutex m;
    bool handlerSignal = false;
    using callback_data = std::tuple<condition_variable_any&, std::mutex&, bool&, decltype(handler)&>;
    callback_data callbackData = {cv, m, handlerSignal, handler};
    auto callback = [](void* callbackData, SDL_Event* event) -> bool {
        auto& [cv, m, handlerSignal, handler] = *reinterpret_cast<callback_data*>(callbackData);
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&handlerSignal](){ return !handlerSignal; });
            handlerSignal = handler(event);
        }
        cv.notify_all();
        return handlerSignal;
    };

    if (!SDL_AddEventWatch(callback, &callbackData)) {
        throw BlsExceptionClass("Failed to add SDL event watch: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
    }

    watchDescriptors.push_back(SdlWatchDescriptor{callback, &callbackData});
    
    while (!stoken.stop_requested()) {
        std::unique_lock lk(m);
        if (cv.wait(lk, stoken, [this, &handlerSignal] { return handlerSignal && !inCooldown(); })) {
            restartCooldown();
            this->sendMessage();
            handlerSignal = false;
        }
        cv.notify_one();
    }
    running.wait(true); // terminate thread once watcher manager has shutdown
}
#endif
    
void DeviceInterruptor::setupThreads() {
    for(auto& idesc : this->device.getIdescList()){
        std::visit(overloads {
            [this](UnixFileInterruptor& idesc) {
                this->globalWatcherThreads.emplace_back(
                    std::bind(&DeviceInterruptor::IFileWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    idesc.file, idesc.interruptCallback
                );
            },
            [this](GpioInterruptor& idesc) {
                this->globalWatcherThreads.emplace_back(
                    std::bind(&DeviceInterruptor::IGpioWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    idesc.portNum, idesc.interruptCallback
                );
            },
            #ifdef SDL_ENABLED
            [this](SdlIoInterruptor& idesc) {
                this->globalWatcherThreads.emplace_back(
                    std::bind(&DeviceInterruptor::ISdlWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2),
                    idesc.interruptCallback
                );
            },
            #endif
        }, idesc);
    }
    watcherManagerThread = std::jthread(std::bind(&DeviceInterruptor::manageWatchers, std::ref(*this), std::placeholders::_1));
}

void DeviceInterruptor::stopThreads() {
    for (auto&& watcher : globalWatcherThreads) {
        watcher.request_stop();
    }
    watcherManagerThread.request_stop();
    globalWatcherThreads.clear();
}