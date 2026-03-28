#pragma once
#include "Devices.hpp"

class DeviceHandle {
    private:
        using device_t = std::variant<
        std::monostate
        #define DEVTYPE_BEGIN(name, ...) \
        , Device::name
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        >;

        device_t device;

        void processStatesImpl(DynamicMessage& input);
    
    public:
        uint16_t id;
        uint32_t cooldown = 0;
        bool isTrigger;

        std::shared_mutex m;
        std::condition_variable_any cv;
        bool processing = false;
        bool watchersPaused = true;
        bool timersPaused = true;

        DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string>& config, std::shared_ptr<ADS7830> targetADC);
        void processStates(DynamicMessage input);
        void init(std::unordered_map<std::string, std::string>& config, std::shared_ptr<ADS7830> targetADC);
        void transmitStates(DynamicMessage& dmsg);
        void transmitDefaultStates(DynamicMessage& dmsg);
        DeviceKind getDeviceKind();
        std::vector<InterruptDescriptor>& getIdescList();
        std::optional<std::thread::id> getLatestCompletedHandlerId(std::stop_token stoken);

};

template<typename T>
class DeviceControlInterface {
    protected:
        DeviceHandle& device;
        std::shared_ptr<Connection> clientConnection;
        int ctl_code;
        int device_code;
    
    private:
        DeviceControlInterface() = delete;
        DeviceControlInterface(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code);
    
        friend T;
};