#include "device_interruptor.hpp"
#include "filenotify/FileNotify.hpp"
#include <boost/range/combine.hpp>
#ifdef __RPI64__
#include <pigpio.h>
#endif

namespace {
    template<class... Ts>
    struct overloads : Ts... { using Ts::operator()...; };
}

DeviceInterruptor::DeviceInterruptor(boost::asio::io_context& in_ctx, DeviceHandle& targDev, std::shared_ptr<Connection> conex, int ctl, int dd)
                                    : DeviceControlInterface(targDev, conex, ctl, dd)
                                    , cooldownTimer(in_ctx) {}

DeviceInterruptor::DeviceInterruptor(DeviceInterruptor&& other)
                                    : 
              DeviceControlInterface(other.device
                                    , other.clientConnection
                                    , other.ctl_code
                                    , other.device_code)
                                    , cooldownTimer(std::move(other.cooldownTimer))
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
    sm.header.kind = device.getDeviceKind(); 

    // Change the timer_id specification: 
    sm.header.timer_id = -1;

    // Volatility does not need to be recorded
    sm.header.volatility = 0; 

    DynamicMessage dmsg; 
    this->device.transmitStates(dmsg); 
    sm.body = dmsg.Serialize(); 

    sm.header.body_size = sm.body.size() ; 

    this->clientConnection->send(sm); 
}

void DeviceInterruptor::disableWatchers() {
    for (auto&& descriptor : watchDescriptors) {
        std::visit(overloads {
            [](FileWatchDescriptor& desc) {
                auto&& [file, callback, _] = desc;
                FileNotify::disableWatch(file);
            },
            [](GpioWatchDescriptor& desc [[ maybe_unused ]]) {
                #ifdef __RPI64__
                auto&& [port, callback, callbackData] = desc;
                gpioSetAlertFuncEx(port, NULL, &callbackData);
                #endif
            },
            #ifdef SDL_ENABLED
            [](SdlWatchDescriptor& desc) {
                auto&& [callback, callbackData] = desc;
                SDL_RemoveEventWatch(callback, &callbackData);
            },
            #endif
            [](HttpWatchDescriptor& desc){
                auto&& [server, endpoint, callback, _] = desc;
                server->removeHttpWatch(endpoint); 
            }
        }, descriptor);
    }
}

void DeviceInterruptor::enableWatchers() {
    for (auto&& descriptor : watchDescriptors) {
        std::visit(overloads {
            [](FileWatchDescriptor& desc) {
                auto&& [file, callback, _] = desc;
                FileNotify::enableWatch(file, callback);
            },
            [](GpioWatchDescriptor& desc [[ maybe_unused ]]) {
                #ifdef __RPI64__
                auto&& [port, callback, callbackData] = desc;
                gpioSetAlertFuncEx(port, callback, &callbackData);
                #endif
            },
            #ifdef SDL_ENABLED
            [](SdlWatchDescriptor& desc) {
                auto&& [callback, callbackData] = desc;
                SDL_AddEventWatch(callback, &callbackData);
            }, 
            #endif
            [](HttpWatchDescriptor&desc){
                auto&& [server, endpoint, callback, _] = desc; 
                server->addHttpWatch(endpoint, callback);
            }

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
        }
    }
    disableWatchers();
    running = false;
    running.notify_all();
}

void DeviceInterruptor::InterruptWatcher(std::stop_token stoken, SynchronizationData& syncPrimitives){
    auto& [cv, m, handlerSignal] = syncPrimitives;
    while (!stoken.stop_requested()) {
        std::unique_lock lk(m);
        if (cv.wait(lk, stoken, [&handlerSignal] { return handlerSignal; })) {
            if (!inCooldown()) {
                restartCooldown();
                this->sendMessage();
            }
            handlerSignal = false;
        }
        cv.notify_one();
    }
    running.wait(true); // terminate thread once watcher manager has shutdown
}

DeviceInterruptor::SynchronizationData& DeviceInterruptor::addFileWatch(std::string filename, std::function<bool()> handler, FileWatchDescriptor& wdesc) {
    wdesc.file = std::filesystem::path(filename).make_preferred();
    wdesc.callback = [&syncPrimitives = wdesc.syncPrimitives, handler](){
        auto& [cv, m, handlerSignal] = syncPrimitives;
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&handlerSignal](){return !handlerSignal; }); 
            handlerSignal = handler(); 
        }
        cv.notify_all(); 
        return handlerSignal;
    };
    FileNotify::addWatch(wdesc.file, wdesc.callback);
    return wdesc.syncPrimitives;
}

DeviceInterruptor::SynchronizationData& DeviceInterruptor::addGpioWatch(int portNum, std::function<bool(int, int, uint32_t)> handler, GpioWatchDescriptor& wdesc) {
    wdesc.port = portNum;
    wdesc.callbackData.second = handler;
    wdesc.callback = [](int gpio, int level, unsigned int tick, void* callbackData) -> void {
        auto& [syncPrimitives, handler] = *reinterpret_cast<decltype(GpioWatchDescriptor::callbackData)*>(callbackData);
        auto& [cv, m, handlerSignal] = syncPrimitives;
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&handlerSignal](){ return !handlerSignal; });
            handlerSignal = handler(gpio, level, tick);
        }
        cv.notify_all();
    };

    #ifdef __RPI64__
    gpioSetAlertFuncEx(wdesc.port, wdesc.callback, &wdesc.callbackData);
    #endif

    return wdesc.callbackData.first;
}

#ifdef SDL_ENABLED
DeviceInterruptor::SynchronizationData& DeviceInterruptor::addSdlWatch(std::function<bool(SDL_Event*)> handler, SdlWatchDescriptor& wdesc) {
    wdesc.callbackData.second = handler;
    wdesc.callback = [](void* callbackData, SDL_Event* event) -> bool {
        auto& [syncPrimitives, handler] = *reinterpret_cast<decltype(SdlWatchDescriptor::callbackData)*>(callbackData);
        auto& [cv, m, handlerSignal] = syncPrimitives;
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&handlerSignal](){ return !handlerSignal; });
            handlerSignal = handler(event);
        }
        cv.notify_all();
        return handlerSignal;
    };

    if (!SDL_AddEventWatch(wdesc.callback, &wdesc.callbackData)) {
        throw BlsExceptionClass("Failed to add SDL event watch: " + std::string(SDL_GetError()), ERROR_T::DEVICE_FAILURE);
    }
    
    return wdesc.callbackData.first;
}
#endif

DeviceInterruptor::SynchronizationData& DeviceInterruptor::addHttpWatch(std::shared_ptr<HttpListener> server, std::string endpoint, std::function<bool(int64_t, std::string, std::string)> handler, HttpWatchDescriptor& wdesc) {
    wdesc.listener = server;
    wdesc.endpoint = endpoint;
    wdesc.callback = [&syncPrimitives = wdesc.syncPrimitives, handler](int64_t sessionID, std::string ip, std::string json){
        auto& [cv, m, handlerSignal] = syncPrimitives;
        {
            std::unique_lock lk(m);
            cv.wait(lk, [&handlerSignal](){return !handlerSignal; }); 
            handlerSignal = handler(sessionID, ip, json); 
        }
        cv.notify_all(); 
        return handlerSignal;
    }; 

    server->addHttpWatch(wdesc.endpoint, wdesc.callback);

    return wdesc.syncPrimitives;
}

    
void DeviceInterruptor::setupThreads() {
    watchDescriptors = std::vector<WatchDescriptor>(this->device.getIdescList().size());
    for(auto&& [idesc, wdesc] : boost::combine(this->device.getIdescList(), watchDescriptors)){
        auto& syncPrimitives = std::visit(overloads {
            [this, &wdesc](FileInterruptor& idesc) -> SynchronizationData& {
                return addFileWatch(idesc.file
                                  , idesc.interruptCallback
                                  , wdesc.emplace<FileWatchDescriptor>());
            },
            [this, &wdesc](GpioInterruptor& idesc) -> SynchronizationData& {
                return addGpioWatch(idesc.portNum
                                  , idesc.interruptCallback
                                  , wdesc.emplace<GpioWatchDescriptor>());
            },
            #ifdef SDL_ENABLED
            [this, &wdesc](SdlIoInterruptor& idesc) -> SynchronizationData& {
                return addSdlWatch(idesc.interruptCallback
                                 , wdesc.emplace<SdlWatchDescriptor>());
            },
            #endif
            [this, &wdesc](HttpInterruptor &idesc) -> SynchronizationData& {
                return addHttpWatch(idesc.server
                                  , idesc.endpoint
                                  , idesc.interruptCallback
                                  , wdesc.emplace<HttpWatchDescriptor>());
            }
            
        }, idesc);
        this->globalWatcherThreads.emplace_back(
            std::bind(&DeviceInterruptor::InterruptWatcher, std::ref(*this), std::placeholders::_1, std::placeholders::_2),
            std::ref(syncPrimitives)
        );
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