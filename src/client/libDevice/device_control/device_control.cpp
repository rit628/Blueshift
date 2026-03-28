#include "device_control.hpp"

namespace {
    template<class... Ts>
    struct overloads : Ts... { using Ts::operator()...; };
}

class DevicePoller;
class DeviceInterruptor;
class DeviceCursor;

DeviceHandle::DeviceHandle(TYPE dtype, std::unordered_map<std::string, std::string>& config, std::shared_ptr<ADS7830> usingADC) 
{
    switch(dtype){
        #define DEVTYPE_BEGIN(name, ...) \
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
    if (getDeviceKind() == DeviceKind::CURSOR) { // skip mutex locking
        processStatesImpl(dmsg);
        return;
    }

    // notify watchers and timers that message processing is about to begin
    {
        std::unique_lock lk(m);
        cv.wait(lk, [this]{ return !processing; }); // wait for previous message to process completely
        processing = true;
    }
    cv.notify_all();

    // wait until watchers and timers are paused then process message
    {
        std::unique_lock lk(m);
        cv.wait(lk, [this]{ return watchersPaused && timersPaused; });
        processStatesImpl(dmsg);
        processing = false;
    }
    cv.notify_all(); // notify all watchers and timers to re-enable
}

void DeviceHandle::init(std::unordered_map<std::string, std::string>& config, std::shared_ptr<ADS7830> adc) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&config, adc](auto& dev) {
            // SET ADC BEFORE INIT ALWAYS
            dev.adc = adc;
            dev.init(config);
        }

    }, device);
}

void DeviceHandle::transmitStates(DynamicMessage& dmsg) {
    std::visit(overloads {
        [](std::monostate&) {},
        [&dmsg](auto& dev) { dev.transmitStates(dmsg); }
    }, device);
}

void DeviceHandle::transmitDefaultStates(DynamicMessage& dmsg) {
    std::visit(overloads {
        [](std::monostate&) -> void { throw std::runtime_error("Attempt to access device kind for null device."); },
        #define DEVTYPE_BEGIN(name, kind) \
        [&dmsg](Device::name& dev [[ maybe_unused ]]) -> void { \
            auto fallback = TypeDef::name(); \
            dmsg.packStates(fallback); \
        },
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
    }, device);
}

DeviceKind DeviceHandle::getDeviceKind() {
    return std::visit(overloads {
        [](std::monostate&) -> DeviceKind { throw std::runtime_error("Attempt to access device kind for null device."); },
        #define DEVTYPE_BEGIN(name, kind) \
        [](Device::name& dev [[ maybe_unused ]]) -> DeviceKind { \
            return DeviceKind::kind; \
        },
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
    }, device);
}

std::vector<InterruptDescriptor>& DeviceHandle::getIdescList() {
    return std::visit(overloads {
        [](std::monostate&) -> std::vector<InterruptDescriptor>& { throw std::runtime_error("Attempt to access Idesc List for null device."); },
        [](auto& dev) -> std::vector<InterruptDescriptor>& { return dev.Idesc_list; }
    }, device);
}

std::optional<std::thread::id> DeviceHandle::getLatestCompletedHandlerId(std::stop_token stoken) {
    return std::visit(overloads {
        [](std::monostate&) -> std::optional<std::thread::id> { throw std::runtime_error("Attempt to access Idesc List for null device."); },
        [&stoken](auto& dev) -> std::optional<std::thread::id> {
            auto lastQueryResult = dev.queryQueue.peek(stoken);
            if (lastQueryResult.has_value()) {
                return lastQueryResult.value().first;
            }
            return std::nullopt;
        }
    }, device);
}

template<typename T>
DeviceControlInterface<T>::DeviceControlInterface(DeviceHandle& device, std::shared_ptr<Connection> clientConnection, int ctl_code, int device_code)
                                                : device(device), clientConnection(clientConnection), ctl_code(ctl_code), device_code(device_code) {}

template class DeviceControlInterface<DevicePoller>;
template class DeviceControlInterface<DeviceInterruptor>;
template class DeviceControlInterface<DeviceCursor>;