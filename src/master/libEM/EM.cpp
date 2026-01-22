#include "EM.hpp"
#include "include/Common.hpp"
#include "MM.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libtype/bls_types.hpp"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <variant>
#include <vector>

// 

ExecutionManager::ExecutionManager(std::vector<TaskDescriptor> TaskList, TSQ<EMStateMessage> &readMM, 
    TSQ<HeapMasterMessage> &sendMM,
    std::vector<char>& bytecode,
    std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> tasks)
    : readMM(readMM), sendMM(sendMM), scheduler(TaskList, [this](HeapMasterMessage dmm){this->sendMM.write(dmm);})
{
    this->TaskList = TaskList;
    for(auto &task : TaskList)
    {
        std::string TaskName = task.name;
        std::vector<std::string> devices;
        std::vector<bool> isVtype;
        std::vector<std::string> controllers;
        for(int j = 0; j < task.binded_devices.size(); j++)
        {
            devices.push_back(task.binded_devices.at(j).device_name);
            isVtype.push_back(task.binded_devices.at(j).isVtype);
            controllers.push_back(task.binded_devices.at(j).controller);
        }

        auto function = tasks[TaskName];
        auto bytecodeOffset = task.bytecode_offset;
        EU_map[TaskName] = std::make_unique<ExecutionUnit>(task, devices, isVtype, controllers, this->sendMM, bytecodeOffset, bytecode, function, this->scheduler, eu_ctx);
    }
}

ExecutionUnit::ExecutionUnit(TaskDescriptor task, std::vector<std::string> devices, std::vector<bool> isVtype, std::vector<std::string> controllers,
    TSQ<HeapMasterMessage> &sendMM, size_t bytecodeOffset, std::vector<char>& bytecode, std::function<std::vector<BlsType>(std::vector<BlsType>)>  transform_function, DeviceScheduler &devScheduler, asio::io_context &ctx)
    : globalScheduler(devScheduler), sendMM(sendMM), ctx(ctx)
{
    this->Task = task;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
    this->vm.setParentExecutionUnit(this);
    this->vm.setTaskOffset(bytecodeOffset);
    this->vm.loadBytecode(bytecode);
    this->transform_function = transform_function;
    this->info.task = task.name;

    // Device Position Map:
    int i = 0;  
    for(auto& device : devices){
        this->devicePositionMap[device] = i; 
        i++; 
    }

    //this->running(vtypeHMMsMap, sendMM);
    this->executionThread = std::thread(&ExecutionUnit::running, this);
}

ExecutionUnit::~ExecutionUnit()
{
    this->executionThread.join();
}

HeapMasterMessage::HeapMasterMessage(std::shared_ptr<HeapDescriptor> heapTree, Task_Info info, PROTOCOLS protocol, bool isInterrupt)
{
    this->heapTree = heapTree;
    this->info = info;
    this->protocol = protocol;
    this->isInterrupt = isInterrupt;
}

DynamicMasterMessage::DynamicMasterMessage(DynamicMessage DM, Task_Info info, PROTOCOLS protocol, bool isInterrupt)
{
    this->DM = DM;
    this->info = info;
    this->protocol = protocol;
    this->isInterrupt = isInterrupt;
}


void ExecutionUnit::replaceCachedStates(std::unordered_map<DeviceID, HeapMasterMessage> &cachedHMMs){

    auto replacementItems = this->replacementCache.getMap(); 
    for(auto& item : replacementItems){
        //std::cout<<"Replacing the cache"<<std::endl; 
        //std::cout<<"REPLACING CACHED STATES: "<<item.first<<std::endl;
        auto devState = item.first;
        HeapMasterMessage replaceHMM = item.second;
        cachedHMMs[devState] = replaceHMM; 
    }
}


void ExecutionUnit::running()
{
    while(true)
    {

        EMStateMessage currentHMMs = EUcache.read();

        this->TriggerName = currentHMMs.TriggerName;  
        //std::cout<<this->Task.name <<" TRIGGERED BY: "<<TriggerName<<std::endl; 

        std::unordered_map<DeviceID, HeapMasterMessage> HMMs;
        
        // Fill in the known data into the stack 
        for(auto &HMM : currentHMMs.dmm_list)
        {   
            HMMs[HMM.info.device] = HMM; 
        }

        this->globalScheduler.request(this->Task.name, currentHMMs.priority); 

        replaceCachedStates(HMMs); 
        
        std::vector<BlsType> transformableStates;

        // Tell the mailbox that the process is in execution
        HeapMasterMessage execMsg;
        execMsg.info.task = this->Task.name;
        execMsg.protocol = PROTOCOLS::PROCESS_EXEC; 
        this->sendMM.write(execMsg);

        int i = 0; 
        for(auto& deviceDesc : this->Task.binded_devices){
            DeviceID devName = deviceDesc.device_name; 

            if(HMMs.contains(devName)){
                auto state = HMMs.at(devName).heapTree;
                if (std::holds_alternative<std::shared_ptr<HeapDescriptor>>(state)) {
                    // make sure default is set to unmodified (may not be needed depending on serialization
                    auto desc = std::get<std::shared_ptr<HeapDescriptor>>(state)->clone();
                    desc->modified = false;
                    desc->index = i;  
                    state = std::move(desc);
                    
                }
                transformableStates.push_back(state); 
            }
            else{
                auto defDevice = deviceDesc.initialValue; 
                if (std::holds_alternative<std::shared_ptr<HeapDescriptor>>(defDevice)) {
                    // make sure default is set to unmodified (may not be needed depending on serialization
                    auto desc = std::get<std::shared_ptr<HeapDescriptor>>(defDevice)->clone();
                    desc->modified = false;
                    desc->index = i; 
                    defDevice = std::move(desc);
                }
                transformableStates.push_back(defDevice); 
            }
            i++; 
        }

        transformableStates = vm.transform(transformableStates);
        auto& modifiedStates = vm.getModifiedStates();

        std::vector<HeapMasterMessage> outGoingStates;  

        // Release before retrieval
        this->globalScheduler.release(this->Task.name);

        for(auto& devDesc : this->Task.outDevices)
        {   
            size_t pos = this->devicePositionMap[devDesc.device_name];
            if (!modifiedStates.at(pos)) continue;
            auto transformedState = transformableStates.at(pos);
            HeapMasterMessage newHMM; 
            newHMM.info.controller = devDesc.controller;
            newHMM.info.device = devDesc.device_name; 
            newHMM.info.isVtype = devDesc.isVtype; 
            newHMM.info.task = this->Task.name; 
            newHMM.protocol = PROTOCOLS::SENDSTATES;
            newHMM.isInterrupt = false; 
            newHMM.heapTree = transformedState;
            newHMM.isCursor = (devDesc.deviceKind == DeviceKind::CURSOR);
            outGoingStates.push_back(newHMM); 
        }

        for(HeapMasterMessage& hmm : outGoingStates)
        {
            this->sendMM.write(hmm);
        }

        // clear the replacement cache since the task ran successfully
        this->replacementCache.clear(); 
    }
}

ExecutionUnit &ExecutionManager::assign(HeapMasterMessage DMM)
{   
    
    ExecutionUnit &assignedUnit = *EU_map.at(DMM.info.task); 
    assignedUnit.stateMap.emplace(DMM.info.device, DMM);
    return assignedUnit;
}

void ExecutionManager::running()
{
    bool started = true; 
    while(1)
    {
        // Send an initial request for data to be stored int the queues
        if(started){
            for(auto& pair : EU_map)
            {
                Task_Info info = pair.second->info;
                //std::cout<<"Requesting States for : "<<info.task<<std::endl;
                HeapMasterMessage requestHMM(nullptr, info, PROTOCOLS::REQUESTINGSTATES, false);
                this->sendMM.write(requestHMM);

            }
            started = false; 
        }
        
        EMStateMessage currentDMMs = this->readMM.read(); 
        ExecutionUnit &assignedUnit = assign(currentDMMs.dmm_list[0]);

        switch(currentDMMs.protocol){
            case(PROTOCOLS::OWNER_GRANT):{
                this->scheduler.receive(currentDMMs.dmm_list[0]); 
                break;
            }
            case(PROTOCOLS::OWNER_CONFIRM_OK):{
                //std::cout<<"Passing owner confirm ok"<<std::endl; 
                this->scheduler.receive(currentDMMs.dmm_list[0]); 
                break; 
            }
            case(PROTOCOLS::WAIT_STATE_FORWARD):{
                HeapMasterMessage dmm = currentDMMs.dmm_list[0]; 
                assignedUnit.replacementCache.insert(dmm.info.device, dmm); 
                break; 
            }
            case(PROTOCOLS::PULL_RESPONSE):{
                HeapMasterMessage hmm = currentDMMs.dmm_list[0];
                assignedUnit.pullVMArguments(hmm); 
                // Allow the task to continue execution by unblocking the syscall
                {
                    std::lock_guard<std::mutex> lock(assignedUnit.pullMutex);
                    assignedUnit.pullCounter--; 
                }
                assignedUnit.pullCV.notify_one();
                break; 
            }
            default:{
                assignedUnit.EUcache.write(currentDMMs);
                break; 
            }

        }   
        
        if(assignedUnit.EUcache.getSize() < 3)
        {   
            Task_Info info = assignedUnit.info;
            HeapMasterMessage requestHMM(nullptr, info, PROTOCOLS::REQUESTINGSTATES, false);
            this->sendMM.write(requestHMM);
        }
    
    }
}

// TRAP IMPLEMENTATIONS

void ExecutionUnit::sendPushState(std::vector<BlsType> typeList){
        for(BlsType blsType : typeList){
            HeapMasterMessage pushStateHmm; 
       
            pushStateHmm.protocol = PROTOCOLS::SENDSTATES;
            int index = 0; 
            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(blsType)){
                index = std::get<std::shared_ptr<HeapDescriptor>>(blsType)->index; 
                pushStateHmm.heapTree = std::get<std::shared_ptr<HeapDescriptor>>(blsType)->clone(); 
            }
            else{
                throw std::runtime_error("Support for pushing to non-primative devices is not yet implemented");
            }

            this->vm.getModifiedStates()[index] = false; 

            // TODO: CHANGE THIS TO USE THE ACTUAL NAME INSTEAD OF THE ALIAS
            pushStateHmm.info.device = this->Task.binded_devices.at(index).device_name; 
            pushStateHmm.info.task = this->Task.name; 
            pushStateHmm.info.controller = this->Task.binded_devices.at(index).controller; 
            pushStateHmm.isCursor = (this->Task.binded_devices.at(index).deviceKind == DeviceKind::CURSOR); 
            this->sendMM.write(pushStateHmm); 
        }
    }


std::vector<BlsType> ExecutionUnit::sendPullState(std::vector<BlsType> typeList){
        this->pullStoreVector.clear(); 
        int numPulls  = typeList.size(); 
        this->pullStoreVector.resize(numPulls); 
        this->pullCounter = numPulls; 
        int i = 0; 
        for(BlsType blsType : typeList){
            HeapMasterMessage pullStateHmm; 
            int index = 0; 
            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(blsType)){
                index = std::get<std::shared_ptr<HeapDescriptor>>(blsType)->index; 
            }
            else{
                throw std::runtime_error("Suppose for pushing to non-primative device is not yet implemented");
            }
            auto& deviceName =  this->Task.binded_devices.at(index).device_name; 
            auto& controllerName =  this->Task.binded_devices.at(index).controller; 

            this->pullPlacement.emplace(deviceName, i); 
            pullStateHmm.protocol = PROTOCOLS::PULL_REQUEST; 
     
            // TODO: CHANGE THIS TO USE THE DEVICE ALIAS MAPPING
            pullStateHmm.info.device = deviceName; 
            pullStateHmm.info.task = this->Task.name; 
            pullStateHmm.info.controller = controllerName; 
            pullStateHmm.isCursor = (this->Task.binded_devices.at(index).deviceKind == DeviceKind::CURSOR); 
            this->sendMM.write(pullStateHmm); 
            i++; 
        }

        {
            std::unique_lock<std::mutex> lock(this->pullMutex);
            this->pullCV.wait(lock, [this](){return this->pullCounter == 0;}); 
        }

        this->pullPlacement.clear(); 
        return this->pullStoreVector; 
    }


// Finish this: 
void ExecutionUnit::pullVMArguments(HeapMasterMessage &hmm){
    DeviceID device = hmm.info.device; 
    int pullPos = this->pullPlacement.at(device);
    this->pullStoreVector.at(pullPos) = hmm.heapTree; 
}

void ExecutionUnit::sendTriggerChange(std::string& triggerID, TaskID& taskID, bool isEnable){
        HeapMasterMessage trigChangeHmm; 
        trigChangeHmm.info.task = taskID; 
        trigChangeHmm.info.device = triggerID; 
        
        if(isEnable){
            trigChangeHmm.protocol = PROTOCOLS::ENABLE_TRIGGER; 
        }   
        else{
            trigChangeHmm.protocol = PROTOCOLS::DISABLE_TRIGGER; 
        }
        
        this->sendMM.write(trigChangeHmm); 
    }



