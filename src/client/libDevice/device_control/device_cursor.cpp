#include "device_cursor.hpp"

DeviceCursor::DeviceCursor(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code)
                          : DeviceControlInterface(device, clientConnection, ctl_code, device_code) {}

DeviceCursor::DeviceCursor(DeviceCursor&& other)
                          :
    DeviceControlInterface(other.device
                          , other.clientConnection
                          , other.ctl_code
                          , other.device_code)
{
    bool wasRunning = other.queryWatcherThread.joinable();
    other.queryWatcherThread.request_stop();
    if (wasRunning) {
        this->initialize();
    }
}

DeviceCursor::~DeviceCursor() {
    this->queryWatcherThread.request_stop();
}

void DeviceCursor::queryWatcher(std::stop_token stoken) {
    while (!stoken.stop_requested()) {
        auto lastCompletedHandler = device.getLatestCompletedHandlerId(stoken);
        if (!lastCompletedHandler.has_value()) {
            break; // stop requested, end watcher
        }
        auto handlerId = lastCompletedHandler.value();
        updateView(queryHandlers.get(handlerId).value());
        queryHandlers.remove(handlerId); // signals query completion
    }
}

void DeviceCursor::updateView(uint16_t taskId) {
    DynamicMessage dmsg;
    this->device.transmitStates(dmsg);
    currentViews.insert(taskId, dmsg);
}

void DeviceCursor::initialize() {
    queryWatcherThread = std::jthread(std::bind(&DeviceCursor::queryWatcher, std::ref(*this), std::placeholders::_1));
}

DynamicMessage DeviceCursor::getLatestTaskView(uint16_t taskId) {
    auto dmsg = currentViews.get(taskId);
    if (!dmsg.has_value()) {
        dmsg.emplace();
        device.transmitDefaultStates(dmsg.value());
    }
    return dmsg.value();
}

void DeviceCursor::addQueryHandler(uint16_t taskId) {
    auto handlerId = std::this_thread::get_id();
    queryHandlers.insert(handlerId, taskId);
}

void DeviceCursor::awaitQueryCompletion(std::stop_token stoken) {
    auto handlerId = std::this_thread::get_id();
    while (!stoken.stop_requested()) {
        if (!queryHandlers.contains(handlerId)) {
            return; // this function should not be waiting that long so its ok to busy wait
        }
    }
}