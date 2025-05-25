#pragma once

#include "libDM/DynamicMessage.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <concepts>
#include <variant>

inline bool sendState() { return true; }

// Configuration information used in sending types
enum class SrcType{
    UNIX_FILE, 
    GPIO, 
}; 

struct Interrupt_Desc {
    SrcType src_type;
    std::variant<std::function<bool()>, std::function<bool(int, int, uint32_t)>> interruptHandle;
    std::string file_src;
    int port_num;
}; 

class DeviceCore {
    protected:
        std::vector<Interrupt_Desc> Idesc_list;
        bool hasInterrupt = false;
        // Interrupt watch
        void addFileIWatch(std::string &fileName, std::function<bool()> handler = sendState);
        void addGPIOIWatch(int gpio_port, std::function<bool(int, int, uint32_t)> interruptHandle);

        friend class AbstractDevice;
};

template<typename T, typename... U>
concept OneOf = (std::same_as<T, U> || ...);

template<typename T>
concept Driveable = OneOf<T
#define DEVTYPE_BEGIN(name) \
, TypeDef::name
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END
>;

template <Driveable T>
class Device : public DeviceCore {
    public: 
        void proc_message(DynamicMessage& dmsg);
        void set_ports(std::unordered_map<std::string, std::string> &src);
        void read_data(DynamicMessage &newMsg);

        friend class AbstractDevice;
};

#define DEVTYPE_BEGIN(name) \
template<> \
class Device<TypeDef::name>;
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END