#pragma once

#include "TSQ.hpp"
#include "Serialization.hpp"
#include "virtual_machine.hpp"
#include "Scheduler.hpp"
#include "bls_types.hpp"
#include "TSM.hpp"
#include <condition_variable>
#include <unordered_map>
#include <thread>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <boost/asio.hpp>

namespace asio = boost::asio; 

class ExecutionUnit
{
    public:
    TaskDescriptor Task;
    Task_Info info;
    std::unordered_map<std::string, HeapMasterMessage> stateMap;
    std::vector<std::string> devices;
    std::vector<bool> isVtype;
    std::vector<std::string> controllers;
    TSQ<EMStateMessage> EUcache;
    std::thread executionThread;
    bool stop = false;
    std::unordered_map<std::string, int> devicePositionMap; 
    DeviceScheduler& globalScheduler; 
    // Contains the states to be replaced whikle the device is waiting for write access
    TSM<DeviceID, HeapMasterMessage> replacementCache;
    std::function<std::vector<BlsType>(std::vector<BlsType>)>  transform_function;
    // Get the trigger name
    std::string TriggerName = "";
    BlsLang::VirtualMachine vm;
    TSQ<HeapMasterMessage> &sendMM; 
    asio::io_context &ctx; 


    // Pulling stuff
    int pullCounter = 0; 
    std::mutex pullMutex; 
    std::condition_variable pullCV;
    std::vector<BlsType> pullStoreVector; 
    std::unordered_map<DeviceID, int> pullPlacement; 

    ExecutionUnit(TaskDescriptor TaskData
                , std::vector<std::string> devices
                , std::vector<bool> isVtype
                , std::vector<std::string> controllers
                , TSQ<HeapMasterMessage> &sendMM
                , size_t bytecodeOffset
                , std::vector<char>& bytecode
                , std::function<std::vector<BlsType>(std::vector<BlsType>)> transform_function
                , DeviceScheduler &devSchedule, 
                asio::io_context &ctx);
    

    
    void running();
    // Replaced cached states while devices are read from
    void replaceCachedStates(std::unordered_map<DeviceID, HeapMasterMessage> &cachedHMMs); 
   
    // OS level Traps (should always be a ptr heap descriptor so fine to take as value)
    void sendPushState(std::vector<BlsType> pushStates);
    std::vector<BlsType> sendPullState(std::vector<BlsType> pullStates);
    void pullVMArguments(HeapMasterMessage &hmm); 
    // For the triggerChange message, the device is the trigger (workaround for now)
    void sendTriggerChange(std::string& triggerID, TaskID& taskID, bool isEnable); 


    ~ExecutionUnit();
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(std::vector<TaskDescriptor> TaskList
                   , TSQ<EMStateMessage> &readMM
                   , TSQ<HeapMasterMessage> &sendMM
                   , std::vector<char>& bytecode
                   , std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> tasks);

    ExecutionUnit &assign(HeapMasterMessage DMM);

    void running();

    TSQ<EMStateMessage> &readMM;
    TSQ<HeapMasterMessage> &sendMM;
    std::unordered_map<std::string, std::unique_ptr<ExecutionUnit>> EU_map;
    std::vector<TaskDescriptor> TaskList;
    DeviceScheduler scheduler; 
    boost::asio::io_context eu_ctx; 
};

