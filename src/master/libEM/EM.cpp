#include "EM.hpp"
#include "include/Common.hpp"
#include "libMM/MM.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libtype/bls_types.hpp"
#include <memory>
#include <variant>

// 

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<EMStateMessage> &readMM, 
    TSQ<HeapMasterMessage> &sendMM,
    std::vector<char>& bytecode,
    std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks)
    : readMM(readMM), sendMM(sendMM), scheduler(OblockList, [this](HeapMasterMessage dmm){this->sendMM.write(dmm);})
{
    this->OblockList = OblockList;
    for(auto &oblock : OblockList)
    {
        string OblockName = oblock.name;
         vector<string> devices;
        vector<bool> isVtype;
        vector<string> controllers;
        for(int j = 0; j < oblock.binded_devices.size(); j++)
        {
            devices.push_back(oblock.binded_devices.at(j).device_name);
            isVtype.push_back(oblock.binded_devices.at(j).isVtype);
            controllers.push_back(oblock.binded_devices.at(j).controller);
        }

        auto function = oblocks[OblockName];
        auto bytecodeOffset = oblock.bytecode_offset;
        EU_map[OblockName] = std::make_unique<ExecutionUnit>(oblock, devices, isVtype, controllers, this->sendMM, bytecodeOffset, bytecode, function, this->scheduler);
    }
}

ExecutionUnit::ExecutionUnit(OBlockDesc oblock, vector<string> devices, vector<bool> isVtype, vector<string> controllers,
    TSQ<HeapMasterMessage> &sendMM, size_t bytecodeOffset, std::vector<char>& bytecode, function<vector<BlsType>(vector<BlsType>)>  transform_function, DeviceScheduler &devScheduler)
    : globalScheduler(devScheduler)
{
    this->Oblock = oblock;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
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
    this->executionThread = thread(&ExecutionUnit::running, this,  ref(sendMM));
}

ExecutionUnit::~ExecutionUnit()
{
    this->executionThread.join();
}

HeapMasterMessage::HeapMasterMessage(shared_ptr<HeapDescriptor> heapTree, O_Info info, PROTOCOLS protocol, bool isInterrupt)
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
        auto devState = item.first;
        HeapMasterMessage replaceHMM = item.second;; 
        cachedHMMs[devState] = replaceHMM; 
   }
}


void ExecutionUnit::running(TSQ<HeapMasterMessage> &sendMM)
{
    while(true)
    {

        EMStateMessage currentHMMs = EUcache.read();

        this->TriggerName = currentHMMs.TriggerName;  

        std::unordered_map<DeviceID, HeapMasterMessage> HMMs;
        
        // Fill in the known data into the stack 
        for(auto &HMM : currentHMMs.dmm_list)
        {   
            HMMs[HMM.info.device] = HMM; 
        }

        this->globalScheduler.request(this->Oblock.name, currentHMMs.priority); 

        replaceCachedStates(HMMs); 
        
        vector<BlsType> transformableStates;

        std::cout<<"Trigger name: "<<TriggerName<<std::endl; 

        for(auto& deviceDesc : this->Oblock.binded_devices){
            DeviceID devName = deviceDesc.device_name; 
            if(HMMs.contains(devName)){
                transformableStates.push_back(HMMs.at(devName).heapTree); 
            }
            else{
                auto defDevice = deviceDesc.initialValue; 
                transformableStates.push_back(defDevice); 
            }
        }

        transformableStates = vm.transform(transformableStates);

        std::vector<HeapMasterMessage> outGoingStates;  

        // SHIP ALL OUTGOING DEVICES (EVEN ONCE NOT ALTERED)
        for(auto& devDesc : this->Oblock.outDevices)
        {   
            int pos = this->devicePositionMap[devDesc.device_name]; 
            auto transformedState = transformableStates.at(pos);
            HeapMasterMessage newHMM; 
            newHMM.info.controller = devDesc.controller;
            newHMM.info.device = devDesc.device_name; 
            newHMM.info.isVtype = devDesc.isVtype; 
            newHMM.info.oblock = this->Oblock.name; 
            newHMM.protocol = PROTOCOLS::SENDSTATES;
            newHMM.isInterrupt = false; 
            newHMM.heapTree = transformedState; 
            
            outGoingStates.push_back(newHMM); 
        }

        for(HeapMasterMessage& hmm : outGoingStates)
        {
            sendMM.write(hmm);
        }

        this->globalScheduler.release(this->Oblock.name); 
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
                std::cout<<"Requesting States for : "<<info.oblock<<std::endl;
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
                this->scheduler.receive(currentDMMs.dmm_list[0]); 
                break; 
            }
            case(PROTOCOLS::WAIT_STATE_FORWARD):{
                HeapMasterMessage dmm = currentDMMs.dmm_list[0]; 
                assignedUnit.replacementCache.insert(dmm.info.device, dmm); 
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

