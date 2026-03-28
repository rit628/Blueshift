#pragma once
#include "device_control.hpp"
#include "TSM.hpp"

class DeviceCursor : public DeviceControlInterface<DeviceCursor> {
    private:
        std::jthread queryWatcherThread;
        TSM<uint16_t, DynamicMessage> currentViews;
        TSM<std::thread::id, uint16_t> queryHandlers;

        void queryWatcher(std::stop_token stoken);
        void updateView(uint16_t taskId);

    public:
        DeviceCursor(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code);
        DeviceCursor(DeviceCursor&& other);
        ~DeviceCursor();
        void initialize();
        DynamicMessage getLatestTaskView(uint16_t taskId);
        void addQueryHandler(uint16_t taskId);
        void awaitQueryCompletion(std::stop_token stoken);
};