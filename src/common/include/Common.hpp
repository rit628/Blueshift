#pragma once
#include "libDM/DynamicMessage.hpp"
#include "libtype/bls_types.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <exception>
#include <stdexcept>
#include <optional>
#include <string>
#include <variant>
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
    /* Binding/Declaration Attributes */
    std::string device_name = "";
    TYPE type = TYPE::NONE;
    std::string controller = "";
    std::unordered_map<std::string, std::string> port_maps = {};
    BlsType initialValue = std::monostate();
    bool isVtype = false;

    /* Oblock Specific Attributes */
    bool dropRead = false;
    bool dropWrite = false;
    int polling_period = 1000;
    bool isConst = true;
    /* 
        If the device is registered as a trigger then the exeuction of 
        the oblock is binded to the arrival of the devices state. 
    */ 
    bool isTrigger = true; 

    /* Driver Configuration Attributes */
    bool isInterrupt = false;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & device_name;
        ar & type;
        ar & controller;
        ar & port_maps;
        ar & initialValue;
        ar & isVtype;
        ar & dropRead;
        ar & dropWrite;
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

    std::string name; 
    std::vector<DeviceDescriptor> binded_devices; 
    int bytecode_offset = 0; 
    //std::vector<DeviceDescriptor> inDevices; 
    //std::vector<DeviceDescriptor> outDevices; 

    // Reading Config

    /*
        dropRead :if true -> only read all recieving states once Oblock execution is finished, drop all others
        dropWrite: if true -> Only write to mailbox with the callback is open: 
    */

    bool dropRead = false; 
    bool dropWrite = false; 
    std::vector<TriggerData> triggers = {};

    // Configuration (all time in milliseconds)

    /*
        Synchronize State: Block until all states of refreshed (true for now)
    */
    bool synchronize_states = false;
    /* 
    If custom descriptor is false then all incoming device states are 
    triggers
    */ 
    bool customTriggers = false; 

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
    BlsType heapTree;

    HeapMasterMessage() = default;
    HeapMasterMessage(std::shared_ptr<HeapDescriptor> heapTree, O_Info info, PROTOCOLS protocol, bool isInterrupt);
    HeapMasterMessage(DynamicMasterMessage &DMM){
        this->heapTree = DMM.DM.toTree();
        this->info = DMM.info; 
        this->isInterrupt = DMM.isInterrupt; 
        this->protocol = DMM.protocol; 
    }

    ~HeapMasterMessage() = default;
};

/*
    The GenericException class automatically disables 
    
*/ 




