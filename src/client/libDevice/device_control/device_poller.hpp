#pragma once
#include "device_control.hpp"

class DevicePoller : public DeviceControlInterface<DevicePoller> {
    private:
        struct DeviceTimer {
            int id;
            int poll_period = -1;
            std::chrono::milliseconds period_time;
            std::unordered_map<std::string, std::deque<float>> attr_history;
            std::unordered_map<std::string, float> vol_map;
            boost::asio::steady_timer timer;
            DevicePoller& manager;
        
            DeviceTimer(uint16_t id, int period, boost::asio::io_context& ctx, DevicePoller& manager);
            std::chrono::milliseconds getRemainingTime();
            float calculateStd(std::deque<float> &data);
            void calcVolMap();
            void timerCallback();
            void setPeriod(int newPeriod);
            void start();
            void pause();
            void resume();
        };

        boost::asio::io_context& ctx;
        std::unordered_map<uint16_t, DeviceTimer> timers;
        std::jthread timerManagerThread;

        void pauseTimers();
        void resumeTimers();
        void manageTimers(std::stop_token stoken);

    public: 
        DevicePoller(boost::asio::io_context& ctx, DeviceHandle& device, std::shared_ptr<Connection> cc, int ctl, int dev);
        DevicePoller(DevicePoller&& other);
        ~DevicePoller();

        void sendMessage(DeviceTimer& timer);
        void setPeriod(uint16_t timerId, int newPeriod);
        void createTimer(uint16_t timerId, int period);
        std::vector<uint16_t> getTimerIds();
        void startTimers();
};