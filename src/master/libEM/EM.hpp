#pragma once

#include "libTSQ/TSQ.hpp"
#include "include/Common.hpp"
#include "../../lang/libvirtual_machine/virtual_machine.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libtype/bls_types.hpp"
#include "libTSM/TSM.hpp"
#include <condition_variable>
#include <unordered_map>
#include <thread>
#include <memory>
#include <string>
#include <vector>
#include <functional>
using namespace std;


class ExecutionUnit
{
    public:
    OBlockDesc Oblock;
    O_Info info;
    unordered_map<string, HeapMasterMessage> stateMap;
    vector<string> devices;
    vector<bool> isVtype;
    vector<string> controllers;
    TSQ<EMStateMessage> EUcache;
    thread executionThread;
    bool stop = false;
    std::unordered_map<std::string, int> devicePositionMap; 
    DeviceScheduler& globalScheduler; 
    // Contains the states to be replaced whikle the device is waiting for write access
    TSM<DeviceID, HeapMasterMessage> replacementCache;
    function<vector<BlsType>(vector<BlsType>)>  transform_function;
    // Get the trigger name
    std::string TriggerName = "";
    BlsLang::VirtualMachine vm;
    TSQ<HeapMasterMessage> &sendMM; 


    // Pulling stuff
    int pullCounter = 0; 
    std::mutex pullMutex; 
    std::condition_variable pullCV;
    std::vector<BlsType> pullStoreVector; 
    std::unordered_map<DeviceID, int> pullPlacement; 



    ExecutionUnit(OBlockDesc OblockData
                , vector<string> devices
                , vector<bool> isVtype
                , vector<string> controllers
                , TSQ<HeapMasterMessage> &sendMM
                , size_t bytecodeOffset
                , std::vector<char>& bytecode
                , function<vector<BlsType>(vector<BlsType>)> transform_function
                , DeviceScheduler &devSchedule);
    

    
    void running();
    // Replaced cached states while devices are read from
    void replaceCachedStates(std::unordered_map<DeviceID, HeapMasterMessage> &cachedHMMs); 
   
    // OS level Traps (should always be a ptr heap descriptor so fine to take as value)
    void sendPushState(std::vector<BlsType> &pushStates);
    std::vector<BlsType> sendPullState(std::vector<BlsType> &pullStates);
    void pullVMArguments(HeapMasterMessage &hmm); 
    // For the triggerChange message, the device is the trigger (workaround for now)
    void sendTriggerChange(std::string& triggerID, OblockID& oblockID, bool isEnable); 


    ~ExecutionUnit();
};




class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList
                   , TSQ<EMStateMessage> &readMM
                   , TSQ<HeapMasterMessage> &sendMM
                   , std::vector<char>& bytecode
                   , std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks);

    ExecutionUnit &assign(HeapMasterMessage DMM);

    void running();

    TSQ<EMStateMessage> &readMM;
    TSQ<HeapMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
    DeviceScheduler scheduler; 
};

