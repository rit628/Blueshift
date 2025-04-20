#pragma once
#include "libTSQ/TSQ.hpp"
#include "libTSM/TSM.hpp"
#include "include/Common.hpp"
#include "libvirtual_machine/virtual_machine.hpp"
#include "libdepgraph/depgraph.hpp"
#include <thread>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;


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

    ExecutionUnit(const string& OblockName, vector<string>& devices, vector<bool>& isVtype, vector<string>& controllers, 
    TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, int bytecodeOffset, 
    vector<DeviceDescriptor> &devList);

    void running( TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);

    BlsLang::VirtualMachine vm; 
    int oblockOffset = 0; 
    std::unordered_map<std::string, int> devicePosMap; 
    std::unordered_map<std::string, DeviceDescriptor> devDescs; 
    
    ~ExecutionUnit();
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, 
        TSQ<DynamicMasterMessage> &sendMM, GlobalContext& depMap);
    ExecutionUnit &assign(DynamicMasterMessage DMM);
    void running();
    TSQ<vector<DynamicMasterMessage>> &readMM;
    TSQ<DynamicMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
    TSM<string, vector<HeapMasterMessage>> vtypeHMMsMap;



    
};

