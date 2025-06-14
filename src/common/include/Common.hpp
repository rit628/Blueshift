#pragma once
#include "libDM/DynamicMessage.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <optional>
#include <string>
#include <vector>

enum class PROTOCOLS
{
    // both: 
    SENDSTATES,

    // EM -> MM
    REQUESTINGSTATES,

    // NM -> MM
    CALLBACKRECIEVED,
   
};

enum class PORTTYPE{
    GPIO, 
    I2C,
    UART, 
    SPI
}; 

struct DeviceDescriptor{
    std::string device_name = ""; 
    TYPE type = TYPE::NONE;

    std::string controller = ""; 
    std::unordered_map<std::string, std::string> port_maps = {}; 

    bool isVtype = false;
    int polling_period = 1000;
    bool isConst = true;
    bool isInterrupt = false;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & device_name;
        ar & type;
        ar & controller;
        ar & port_maps;
        ar & isVtype;
        ar & polling_period;
        ar & isConst;
        ar & isInterrupt;
    }

    bool operator==(const DeviceDescriptor&) const = default;
}; 

struct TriggerData {
    std::vector<std::string> rule;
    std::optional<std::string> id = std::nullopt;
    uint8_t priority = 1;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & rule;
        bool hasId = id.has_value();
        ar & hasId;
        if (hasId) {
            id = "";
            ar & id.value();
        }
        ar & priority;
    }

    bool operator==(const TriggerData&) const = default;
};

struct OBlockDesc{

    /*
        Normal State
    */

    std::string name; 
    std::vector<DeviceDescriptor> binded_devices; 
    int bytecode_offset = 0; 

    // Reading Config

    /*
        dropRead :if true -> only read all recieving states once Oblock execution is finished, drop all others
        dropWrite: if true -> Only write to mailbox with the callback is open: 
    */

    bool dropRead = false; 
    bool dropWrite = false; 
    std::vector<TriggerData> triggers;

    // Configuration (all time in milliseconds)

    /*
        Synchronize State: Block until all states of refreshed (true for now)
    */
    bool synchronize_states = true;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & name;
        ar & binded_devices;
        ar & bytecode_offset;
        ar & dropRead;
        ar & dropWrite;
        ar & triggers;
        ar & synchronize_states;
    }

    bool operator==(const OBlockDesc&) const = default;
}; 

struct O_Info
{
    std::string oblock;
    std::string device;
    std::string controller;
    bool isVtype = false;
};

struct DynamicMasterMessage
{
    public:
    O_Info info;
    DynamicMessage DM;
    bool isInterrupt;
    PROTOCOLS protocol;
    DynamicMasterMessage() = default;
    DynamicMasterMessage(DynamicMessage DM, O_Info info, PROTOCOLS protocol, bool isInterrupt);
    ~DynamicMasterMessage() = default;
};

struct HeapMasterMessage
{
    public:
    O_Info info;
    PROTOCOLS protocol;
    bool isInterrupt;
    std::shared_ptr<HeapDescriptor> heapTree;


    HeapMasterMessage() = default;
    HeapMasterMessage(std::shared_ptr<HeapDescriptor> heapTree, O_Info info, PROTOCOLS protocol, bool isInterrupt);
    ~HeapMasterMessage() = default;
};
