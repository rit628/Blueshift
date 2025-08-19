#include "EM.hpp"
#include "include/Common.hpp"
#include "libMM/MM.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libtype/bls_types.hpp"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <variant>
#include <vector>

// 

ExecutionManager::ExecutionManager(std::vector<OBlockDesc> OblockList, TSQ<EMStateMessage> &readMM, 
    TSQ<HeapMasterMessage> &sendMM,
    std::vector<char>& bytecode,
    std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks)
    : readMM(readMM), sendMM(sendMM), scheduler(OblockList, [this](HeapMasterMessage dmm){this->sendMM.write(dmm);})
{
    this->OblockList = OblockList;
    for(auto &oblock : OblockList)
    {
        std::string OblockName = oblock.name;
        std::vector<std::string> devices;
        std::vector<bool> isVtype;
        std::vector<std::string> controllers;
        for(int j = 0; j < oblock.binded_devices.size(); j++)
        {
            devices.push_back(oblock.binded_devices.at(j).device_name);
            isVtype.push_back(oblock.binded_devices.at(j).isVtype);
            controllers.push_back(oblock.binded_devices.at(j).controller);
        }

        auto function = oblocks[OblockName];
        auto bytecodeOffset = oblock.bytecode_offset;
        EU_map[OblockName] = std::make_unique<ExecutionUnit>(oblock, devices, isVtype, controllers, this->sendMM, bytecodeOffset, bytecode, function, this->scheduler, eu_ctx);
    }
}

ExecutionUnit::ExecutionUnit(OBlockDesc oblock, std::vector<std::string> devices, std::vector<bool> isVtype, std::vector<std::string> controllers,
    TSQ<HeapMasterMessage> &sendMM, size_t bytecodeOffset, std::vector<char>& bytecode, std::function<std::vector<BlsType>(std::vector<BlsType>)>  transform_function, DeviceScheduler &devScheduler, asio::io_context &ctx)
    : globalScheduler(devScheduler), sendMM(sendMM), ctx(ctx)
{
    this->Oblock = oblock;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
    this->vm.setParentExecutionUnit(this);
    this->vm.setOblockOffset(bytecodeOffset);
    this->vm.loadBytecode(bytecode);
    this->transform_function = transform_function;
    this->info.oblock = oblock.name;

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

HeapMasterMessage::HeapMasterMessage(std::shared_ptr<HeapDescriptor> heapTree, O_Info info, PROTOCOLS protocol, bool isInterrupt)
{
    this->heapTree = heapTree;
    this->info = info;
    this->protocol = protocol;
    this->isInterrupt = isInterrupt;
}

DynamicMasterMessage::DynamicMasterMessage(DynamicMessage DM, O_Info info, PROTOCOLS protocol, bool isInterrupt)
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
        //std::cout<<this->Oblock.name <<" TRIGGERED BY: "<<TriggerName<<std::endl; 

        std::unordered_map<DeviceID, HeapMasterMessage> HMMs;
        
        // Fill in the known data into the stack 
        for(auto &HMM : currentHMMs.dmm_list)
        {   
            HMMs[HMM.info.device] = HMM; 
        }

        this->globalScheduler.request(this->Oblock.name, currentHMMs.priority); 

        replaceCachedStates(HMMs); 
        
        std::vector<BlsType> transformableStates;

        // Tell the mailbox that the process is in execution
        HeapMasterMessage execMsg;
        execMsg.info.oblock = this->Oblock.name;
        execMsg.protocol = PROTOCOLS::PROCESS_EXEC; 
        this->sendMM.write(execMsg);

        int i = 0; 
        for(auto& deviceDesc : this->Oblock.binded_devices){
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
        this->globalScheduler.release(this->Oblock.name);

        for(auto& devDesc : this->Oblock.outDevices)
        {   
            size_t pos = this->devicePositionMap[devDesc.device_name];
            if (!modifiedStates.at(pos)) continue;
            auto transformedState = transformableStates.at(pos);
            HeapMasterMessage newHMM; 
            newHMM.info.controller = devDesc.controller;
            newHMM.info.device = devDesc.device_name; 
            newHMM.info.isVtype = devDesc.isVtype; 
            newHMM.info.oblock = this->Oblock.name; 
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

        // clear the replacement cache since the oblock ran successfully
        this->replacementCache.clear(); 
    }
}

ExecutionUnit &ExecutionManager::assign(HeapMasterMessage DMM)
{   
    
    ExecutionUnit &assignedUnit = *EU_map.at(DMM.info.oblock); 
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
                O_Info info = pair.second->info;
                //std::cout<<"Requesting States for : "<<info.oblock<<std::endl;
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
                // Allow the oblock to continue execution by unblocking the syscall
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
            O_Info info = assignedUnit.info;
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
            pushStateHmm.info.device = this->Oblock.binded_devices.at(index).device_name; 
            pushStateHmm.info.oblock = this->Oblock.name; 
            pushStateHmm.info.controller = this->Oblock.binded_devices.at(index).controller; 
            pushStateHmm.isCursor = (this->Oblock.binded_devices.at(index).deviceKind == DeviceKind::CURSOR); 
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
            auto& deviceName =  this->Oblock.binded_devices.at(index).device_name; 
            auto& controllerName =  this->Oblock.binded_devices.at(index).controller; 

            this->pullPlacement.emplace(deviceName, i); 
            pullStateHmm.protocol = PROTOCOLS::PULL_REQUEST; 
     
            // TODO: CHANGE THIS TO USE THE DEVICE ALIAS MAPPING
            pullStateHmm.info.device = deviceName; 
            pullStateHmm.info.oblock = this->Oblock.name; 
            pullStateHmm.info.controller = controllerName; 
            pullStateHmm.isCursor = (this->Oblock.binded_devices.at(index).deviceKind == DeviceKind::CURSOR); 
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

void ExecutionUnit::sendTriggerChange(std::string& triggerID, OblockID& oblockID, bool isEnable){
        HeapMasterMessage trigChangeHmm; 
        trigChangeHmm.info.oblock = oblockID; 
        trigChangeHmm.info.device = triggerID; 
        
        if(isEnable){
            trigChangeHmm.protocol = PROTOCOLS::ENABLE_TRIGGER; 
        }   
        else{
            trigChangeHmm.protocol = PROTOCOLS::DISABLE_TRIGGER; 
        }
        
        this->sendMM.write(trigChangeHmm); 
    }



