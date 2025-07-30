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
    OWNER_CANDIDATE_REQUEST_CONCLUDE, 
    OWNER_CONFIRM, 
    OWNER_RELEASE, 
    PROCESS_EXEC,
    DISABLE_TRIGGER,
    


    // NM -> MM
    CALLBACKRECIEVED,

    //MM -> EM
    OWNER_GRANT, 
    OWNER_CONFIRM_OK, 
    CALLBACK_UPDATE, 
    QUEUE_EMPTY, 

    // MM->EM (Forward data to a waiting device)
    WAIT_STATE_FORWARD
   

};

enum class READ_POLICIES{
    ALL,
    ANY,
};

enum class OVERWRITE_POLICIES{
    CLEAR, 
    CURRENT, 
    DISCARD,
    DEFAULT, 
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

    // Policy Codes
    READ_POLICIES readPolicy = READ_POLICIES::ALL;
    OVERWRITE_POLICIES overwritePolicy = OVERWRITE_POLICIES::DEFAULT;
    bool isYield = true; 

    /* Oblock Specific Attributes */
    bool dropRead = false;
    bool dropWrite = false;
    int polling_period = 1000;
    bool isConst = true;
    /* 
        If the device is registered as a trigger then the exeuction of 
        the oblock is binded to the arrival of the devices state. 
    */ 

    /* Driver Configuration Attributes */
    bool isInterrupt = false;
    bool isCursor = false;

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
        ar & isCursor;
    }

    bool operator==(const DeviceDescriptor&) const = default;
}; 

struct TriggerData {
    std::vector<std::string> rule;
    std::string id = "";
    uint16_t priority = 1;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & rule;
        ar & id;
        ar & priority;
    }

    bool operator==(const TriggerData&) const = default;
};

struct OBlockDesc{

    std::string name; 
    std::vector<DeviceDescriptor> binded_devices; 
    int bytecode_offset = 0; 
    std::vector<DeviceDescriptor> inDevices;
    std::vector<DeviceDescriptor> outDevices; 
    std::string hostController = "MASTER";

    std::vector<TriggerData> triggers = {};

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & name;
        ar & binded_devices;
        ar & bytecode_offset;
        ar & inDevices;
        ar & outDevices;
        ar & hostController;
        ar & triggers;
    }

    bool operator==(const OBlockDesc&) const = default;
};

struct O_Info
{
    std::string oblock;
    std::string device;
    std::string controller;
    bool isVtype = false;
    int priority; 
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

struct EMStateMessage{
    std::string TriggerName; 
    OblockID oblockName; 
    std::vector<HeapMasterMessage> dmm_list; 
    int priority; 
    PROTOCOLS protocol; 
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




