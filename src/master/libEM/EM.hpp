#pragma once

#include "../libTSQ/TSQ.hpp"
#include "../libDm/DynamicMessage.hpp"
#include <unordered_map>
#include <map>
#include <memory>
#include <string>
#include <vector>
using namespace std;

enum class DEVTYPE{
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
    std::vector<std::pair<PORTTYPE, int>> port_maps; 

    bool isInterrupt; 

    // May not need to be used: 
    bool isVtype; 
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
        Max pollrate: corresponds to the polling rate of all polling devices binded to the oblock
        Const Poll: Is the polling rate constant (true until ticker table implementation)
        Synchronize State: Block until all states of refreshed (true for now)
    
    */
    int max_pollrate = 500; 
    bool const_poll = true; 
    bool synchronize_states = true; 
}; 

struct O_Info
{
    string oblock;
    string device;
    string controller;
    bool isVtype;
};

struct DynamicMasterMessage
{
    public:
    O_Info info;
    DynamicMessage DM;
    DynamicMasterMessage() = default;
    ~DynamicMasterMessage() = default;
};

struct HeapMasterMessage
{
    public:
    O_Info info;
    //DynamicMessage HM;
    shared_ptr<HeapDescriptor> heapTree;
    HeapMasterMessage() = default;
    HeapMasterMessage(shared_ptr<HeapDescriptor> heapTree, O_Info info);
    ~HeapMasterMessage() = default;
};

class ExecutionUnit
{
    public:
    string OblockName;
    O_Info info;
    unordered_map<string, DynamicMasterMessage> stateMap;
    vector<string> devices;
    vector<bool> isVtype;
    vector<string> controllers;
    ExecutionUnit() = default;
    ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers);
    vector<HeapMasterMessage> process(DynamicMasterMessage DMM, ExecutionUnit unit);
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<DynamicMasterMessage> &in, TSQ<HeapMasterMessage> &out);
    ExecutionUnit &assign(DynamicMasterMessage DMM);
    void running(TSQ<DynamicMasterMessage> &in);
    TSQ<DynamicMasterMessage> &in;
    TSQ<HeapMasterMessage> &out;
    unordered_map<string, ExecutionUnit> EU_map;
    vector<OBlockDesc> OblockList;
    vector<HeapMasterMessage> vtypeHMMs;
    vector<shared_ptr<HeapDescriptor>> transformState(vector<shared_ptr<HeapDescriptor>> HMM_List);
};

