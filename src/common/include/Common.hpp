#pragma once
#include "libDM/DynamicMessage.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <exception>
#include <stdexcept>
#include <unordered_set>

using ControllerID = std::string; 
using DeviceID = std::string; 
using OblockID = std::string; 


enum class PROTOCOLS
{
    // both: 
    SENDSTATES,

    // EM -> MM
    REQUESTINGSTATES,
    OWNER_CANDIDATE_REQUEST, 
    OWNER_CONFIRM, 
    OWNER_RELEASE, 
  
    // NM -> MM
    CALLBACKRECIEVED,

    //MM -> EM
    OWNER_GRANT, 
    // MM->EM (Forward data to a waiting device)
    WAIT_STATE_FORWARD
   

};

enum class PORTTYPE{
    GPIO, 
    I2C,
    UART, 
    SPI
}; 


struct DeviceDescriptor{
    std::string device_name; 
    DEVTYPE devtype; 

    std::string controller; 
    std::unordered_map<std::string, std::string> port_maps; 

    /*
        dropRead :if true -> only read all recieving states once Oblock execution is finished, drop all others
        dropWrite: if true -> Only write to mailbox with the callback is open: 
    */

    bool dropRead = true; 
    bool dropWrite = true; 
    bool isInterrupt = false; 
    bool isVtype = false; 
    bool isConst = true; 
    bool isCursor = true; 
    
    /* 
        If the device is registered as a trigger then the exeuction of 
        the oblock is binded to the arrival of the devices state. 
    */ 
    bool isTrigger = true; 

    int polling_period = 1000;
    

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & device_name;
        ar & devtype;
        ar & controller;
        ar & port_maps;
        ar & isInterrupt;
        ar & isVtype;
        ar & isConst;
        ar & polling_period;
    }

    bool operator==(const DeviceDescriptor&) const = default;
}; 

struct OBlockDesc{

    std::string name; 
    std::vector<DeviceDescriptor> binded_devices; 
    int bytecode_offset; 
    std::vector<DeviceDescriptor> inDevices; 
    std::vector<DeviceDescriptor> outDevices; 
    std::vector<std::vector<std::string>> triggerRules; 
    
    // Keeping this for the compiler but it should be removed: 
    bool dropRead = false; 
    bool dropWrite = false; 
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

struct EMStateMessage{
    std::string TriggerName; 
    OblockID oblockName; 
    std::vector<DynamicMasterMessage> dmm_list; 
    int priority; 
    PROTOCOLS protocol; 


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


/*
    Schdueling issue
*/

// ScheduleRequestMetadata
enum class PROCSTATE{
    WAITING, 
    EXECUTED, 
    LOADED, 
}; 


// Scheduler Request State
struct SchedulerReq{
    OblockID requestorOblock; 
    DeviceID targetDevice; 
    int priority; 
    PROCSTATE ps;     
    int cyclesWaiting; 
    ControllerID ctl; 
}; 

// Req Scheduler used by 
struct ReqComparator{
    bool operator()(const SchedulerReq& a, const SchedulerReq& b) const {
        return a.priority > b.priority; 
    }
}; 




