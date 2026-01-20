#pragma once
#include "libDM/DynamicMessage.hpp"
#include "libtype/bls_types.hpp"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <cstdint>
#include <memory>
#include <queue> 
#include <string>
#include <variant>
#include <vector>
#include <boost/json.hpp>

using ControllerID = std::string; 
using DeviceID = std::string; 
using TaskID = std::string; 


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
    OWNER_RELEASE_NULL, 
    PROCESS_EXEC,
    DISABLE_TRIGGER,
    ENABLE_TRIGGER, 
    PUSH_REQUEST, 
    PULL_REQUEST, 
    PULL_RESPONSE, 

    // NM -> MM
    CALLBACKRECIEVED,

    //MM -> EM
    OWNER_GRANT, 
    OWNER_CONFIRM_OK, 
    QUEUE_EMPTY, 

    // MM->EM (Forward data to a waiting device)
    WAIT_STATE_FORWARD
   
};

enum class READ_POLICY : uint8_t {
    ALL,
    ANY,
};

enum class OVERWRITE_POLICY : uint8_t {
    CLEAR, 
    CURRENT, 
    DISCARD,
    NONE, 
};

enum class PORTTYPE : uint8_t {
    GPIO, 
    I2C,
    UART, 
    SPI
}; 

enum class DeviceKind : uint8_t {
    POLLING,
    INTERRUPT,
    CURSOR,
    ACTUATOR
};

struct DeviceDescriptor {
    /* Binding/Declaration Attributes */
    std::string device_name = "";
    TYPE type = TYPE::NONE;
    std::string controller = "";
    std::unordered_map<std::string, std::string> port_maps = {};
    BlsType initialValue = std::monostate();
    bool isVtype = false;
    
    /* Task Specific Attributes */
    // Policy Codes
    READ_POLICY readPolicy = READ_POLICY::ALL;
    OVERWRITE_POLICY overwritePolicy = OVERWRITE_POLICY::NONE;
    bool isYield = true; 
    int polling_period = -1; // dynamic polling doesnt need a period
    bool isConst = true; // disable dynamic polling subsystem for now, default to 1000ms poll in client
    bool ignoreWriteBacks = false; 
    /* 
        If the device is registered as a trigger then the execution of 
        the task is binded to the arrival of the devices state. 
    */ 

    /* Driver Configuration Attributes */
    DeviceKind deviceKind = DeviceKind::POLLING;

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version) {
        ar & device_name;
        ar & type;
        ar & controller;
        ar & port_maps;
        ar & initialValue;
        ar & isVtype;
        ar & readPolicy;
        ar & overwritePolicy;
        ar & isYield;
        ar & polling_period;
        ar & isConst;
        ar & ignoreWriteBacks;
        ar & deviceKind;
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

struct TaskDescriptor {

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

    bool operator==(const TaskDescriptor&) const = default;
};

struct Task_Info
{
    std::string task;
    std::string device;
    std::string controller;
    bool isVtype = false;
    int priority; 
};

struct DynamicMasterMessage
{
    public:
    Task_Info info;
    DynamicMessage DM;
    bool isInterrupt = false;
    bool isCursor = false; 
    PROTOCOLS protocol;
    int pushID = 0;

    DynamicMasterMessage() = default;
    DynamicMasterMessage(DynamicMessage DM, Task_Info info, PROTOCOLS protocol, bool isInterrupt);
    
    ~DynamicMasterMessage() = default;
};


struct HeapMasterMessage
{
    public:
    Task_Info info;
    PROTOCOLS protocol;
    bool isInterrupt = false;
    bool isCursor = false; 
    BlsType heapTree;
    int pushID = 0;


    HeapMasterMessage() = default;
    HeapMasterMessage(std::shared_ptr<HeapDescriptor> heapTree, Task_Info info, PROTOCOLS protocol, bool isInterrupt);
    HeapMasterMessage(DynamicMasterMessage &DMM){
        this->heapTree = DMM.DM.toTree();
        this->info = DMM.info; 
        this->isInterrupt = DMM.isInterrupt; 
        this->protocol = DMM.protocol; 
    }

    DynamicMasterMessage buildDMM(){
        DynamicMasterMessage dmm;
        dmm.info = this->info; 
        dmm.isCursor = this->isCursor; 
        dmm.isInterrupt = this->isInterrupt; 
        dmm.protocol = this->protocol; 
        dmm.pushID = this->pushID; 
        if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(this->heapTree)){
            dmm.DM.makeFromRoot(std::get<std::shared_ptr<HeapDescriptor>>(this->heapTree)); 
        }
        return dmm; 
    }


    ~HeapMasterMessage() = default;
};



struct EMStateMessage{
    std::string TriggerName; 
    TaskID taskName; 
    std::vector<HeapMasterMessage> dmm_list; 
    int priority; 
    PROTOCOLS protocol; 
    int pushID; 
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
    TaskID requestorTask; 
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

// Receives the queue for each controller
template <typename rtype, typename comp>
struct ControllerQueue{
    private:  
        std::priority_queue<rtype, std::vector<rtype>, comp> schepq; 
    public: 
        bool currOwned = false; 
        std::priority_queue<rtype, std::vector<rtype>, comp>& getQueue(){
            return this->schepq; 
        }
};

inline void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, DeviceDescriptor const & desc) {
    using namespace boost::json;
    auto& obj = jv.emplace_object();
    obj.emplace("device_name", value_from(desc.device_name));
    obj.emplace("type", value_from(static_cast<uint32_t>(desc.type)));
    obj.emplace("controller", value_from(desc.controller));
    obj.emplace("port_maps", value_from(desc.port_maps));
    obj.emplace("initialValue", value_from(desc.initialValue));
    obj.emplace("isVtype", value_from(desc.isVtype));
    obj.emplace("readPolicy", value_from(static_cast<uint8_t>(desc.readPolicy)));
    obj.emplace("overwritePolicy", value_from(static_cast<uint8_t>(desc.overwritePolicy)));
    obj.emplace("isYield", value_from(desc.isYield));
    obj.emplace("polling_period", value_from(desc.polling_period));
    obj.emplace("isConst", value_from(desc.isConst));
    obj.emplace("ignoreWriteBacks", value_from(desc.ignoreWriteBacks));
    obj.emplace("deviceKind", value_from(static_cast<uint8_t>(desc.deviceKind)));
}

inline DeviceDescriptor tag_invoke(const boost::json::value_to_tag<DeviceDescriptor>&, boost::json::value const& jv) {
    using namespace boost::json;
    auto& obj = jv.as_object();
    DeviceDescriptor desc;
    desc.device_name = value_to<std::string>(obj.at("device_name"));
    desc.type = static_cast<TYPE>(value_to<uint32_t>(obj.at("type")));
    desc.controller = value_to<std::string>(obj.at("controller"));
    desc.port_maps = value_to<std::unordered_map<std::string, std::string>>(obj.at("port_maps"));
    desc.initialValue = value_to<BlsType>(obj.at("initialValue"));
    desc.isVtype = value_to<bool>(obj.at("isVtype"));
    desc.readPolicy = static_cast<READ_POLICY>(value_to<uint8_t>(obj.at("readPolicy")));
    desc.overwritePolicy = static_cast<OVERWRITE_POLICY>(value_to<uint8_t>(obj.at("overwritePolicy")));
    desc.isYield = value_to<bool>(obj.at("isYield"));
    desc.polling_period = value_to<int>(obj.at("polling_period"));
    desc.isConst = value_to<bool>(obj.at("isConst"));
    desc.ignoreWriteBacks = value_to<bool>(obj.at("ignoreWriteBacks"));
    desc.deviceKind = static_cast<DeviceKind>(value_to<uint8_t>(obj.at("deviceKind")));
    return desc;
}

inline void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, TriggerData const & trigger) {
    using namespace boost::json;
    auto& obj = jv.emplace_object();
    obj.emplace("rule", value_from(trigger.rule));
    obj.emplace("id", value_from(trigger.id));
    obj.emplace("priority", value_from(trigger.priority));
}

inline TriggerData tag_invoke(const boost::json::value_to_tag<TriggerData>&, boost::json::value const& jv) {
    using namespace boost::json;
    auto& obj = jv.as_object();
    TriggerData trigger;
    trigger.rule = value_to<std::vector<std::string>>(obj.at("rule"));
    trigger.id = value_to<std::string>(obj.at("id"));
    trigger.priority = value_to<uint8_t>(obj.at("priority"));
    return trigger;
}

inline void tag_invoke(const boost::json::value_from_tag&, boost::json::value& jv, TaskDescriptor const & desc) {
    using namespace boost::json;
    auto& obj = jv.emplace_object();
    obj.emplace("name", value_from(desc.name));
    obj.emplace("binded_devices", value_from(desc.binded_devices));
    obj.emplace("bytecode_offset", value_from(desc.bytecode_offset));
    obj.emplace("inDevices", value_from(desc.inDevices));
    obj.emplace("outDevices", value_from(desc.outDevices));
    obj.emplace("hostController", value_from(desc.hostController));
    obj.emplace("triggers", value_from(desc.triggers));
}

inline TaskDescriptor tag_invoke(const boost::json::value_to_tag<TaskDescriptor>&, boost::json::value const& jv) {
    using namespace boost::json;
    auto& obj = jv.as_object();
    TaskDescriptor desc;
    desc.name = value_to<std::string>(obj.at("name"));
    desc.binded_devices = value_to<std::vector<DeviceDescriptor>>(obj.at("binded_devices"));
    desc.bytecode_offset = value_to<int>(obj.at("bytecode_offset"));
    desc.inDevices = value_to<std::vector<DeviceDescriptor>>(obj.at("inDevices"));
    desc.outDevices = value_to<std::vector<DeviceDescriptor>>(obj.at("outDevices"));
    desc.hostController = value_to<std::string>(obj.at("hostController"));
    desc.triggers = value_to<std::vector<TriggerData>>(obj.at("triggers"));
    return desc;
}
