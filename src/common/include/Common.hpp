#pragma once
#include "libHD/HeapDescriptors.hpp"
#include "libDM/DynamicMessage.hpp"

enum class PROTOCOLS
{
    // both: 
    SENDSTATES,

    // EM -> MM
    REQUESTINGSTATES,

    // NM -> MM
    CALLBACKRECIEVED,
   
};

enum class DEVTYPE : uint16_t{
    SERVO, 
    LIGHT, 
    STROBE_LIGHT,
    MOTOR, 
    HUMID, 
    BUTTON, 
    POTENT, 
    PIEZO, 

    // Virtual Type Primatives (Built in)
    VINT, 
    VFLOAT, 
    VSTRING,

    // DEBUG devices for timers and interupts
    TIMER_TEST, 
    LINE_WRITER, 
    TIMER_TEST_NUM
    
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

    bool isInterrupt; 
    bool isVtype; 
    bool isConst; 
    int polling_period; 
}; 


struct OBlockDesc{

    /*
        Normal State
    */

    std::string name; 
    std::vector<DeviceDescriptor> binded_devices; 
    int bytecode_offset; 

    // Reading Config

    /*
        dropRead :if true -> only read all recieving states once Oblock execution is finished, drop all others
        dropWrite: if true -> Only write to mailbox with the callback is open: 
    */

    bool dropRead; 
    bool dropWrite; 

    // Configuration (all time in milliseconds)

    /*
        Synchronize State: Block until all states of refreshed (true for now)
    */
    bool synchronize_states = true; 
}; 

struct O_Info
{
    std::string oblock;
    std::string device;
    std::string controller;
    bool isVtype;
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
