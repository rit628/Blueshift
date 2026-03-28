#include "device_poller.hpp"
#include <boost/range/adaptor/map.hpp>

DevicePoller::DeviceTimer::DeviceTimer(uint16_t id, int period, boost::asio::io_context& ctx, DevicePoller& manager)
                                     : id(id), poll_period(period), timer(ctx), manager(manager) {}

std::chrono::milliseconds DevicePoller::DeviceTimer::getRemainingTime() {
    auto now_time = std::chrono::steady_clock::now(); 
    auto timer_exp = timer.expires_at();

    if(now_time < timer_exp){
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
                         : DeviceControlInterface(device, cc, ctl, dev)
                         , ctx(in_ctx) {}

DevicePoller::DevicePoller(DevicePoller&& other)
                         :
    DeviceControlInterface(other.device
                         , other.clientConnection
                         , other.ctl_code
                         , other.device_code)
                         , ctx(other.ctx)

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
        }
    }
    pauseTimers();
}

void DevicePoller::sendMessage(DeviceTimer& timer) {
    SentMessage smsg; 

    DynamicMessage dmsg; 
    this->device.transmitStates(dmsg); 

    // Extract numerical data out the fields and add to the src: 
    dmsg.getFieldVolatility(timer.attr_history); 

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
    smsg.header.kind = device.getDeviceKind(); 

    this->clientConnection->send(smsg);
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