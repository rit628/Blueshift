#pragma once

#include "../libTSQ/TSQ.hpp"
#include "../libTSM/TSM.hpp"
#include "../libDM/DynamicMessage.hpp"
//#include "../../lang/libinterpreter/interpreter.hpp"
//#include "../../lang/common/ast.hpp"
#include <unordered_map>
#include <thread>
#include <memory>
#include <string>
#include <vector>
using namespace std;

enum class PROTOCOLS
{
    SENDSTATES,
    CALLBACKRECIEVED,
    REQUESTINGSTATES,
};

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
    shared_ptr<HeapDescriptor> heapTree;
    HeapMasterMessage() = default;
    HeapMasterMessage(shared_ptr<HeapDescriptor> heapTree, O_Info info, PROTOCOLS protocol, bool isInterrupt);
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
    TSQ<vector<DynamicMasterMessage>> EUcache;
    thread executionThread;
    bool stop = false;
    ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers, 
        TSM<string, HeapMasterMessage> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);
    void running(TSM<string, HeapMasterMessage> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);
    vector<shared_ptr<HeapDescriptor>> transformState(vector<shared_ptr<HeapDescriptor>> HMM_List);
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, TSQ<DynamicMasterMessage> &sendMM);
    ExecutionUnit &assign(DynamicMasterMessage DMM);
    void running(TSQ<vector<DynamicMasterMessage>> &readMM);
    TSQ<vector<DynamicMasterMessage>> &readMM;
    TSQ<DynamicMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
    vector<HeapMasterMessage> vtypeHMMs;
    TSM<string, HeapMasterMessage> vtypeHMMsMap;
    //BlsLang::Interpreter interpreter;
    //BlsLang::AstNode ast;
};

