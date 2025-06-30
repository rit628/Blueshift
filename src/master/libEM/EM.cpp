#include "EM.hpp"
#include "include/Common.hpp"
#include "libMM/MM.hpp"
#include "libtype/bls_types.hpp"
#include <memory>

// 

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<HeapMasterMessage>> &readMM, 
    TSQ<HeapMasterMessage> &sendMM, 
    std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks)
    : readMM(readMM), sendMM(sendMM)
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
        EU_map[OblockName] = std::make_unique<ExecutionUnit>(oblock, devices, isVtype, controllers, this->sendMM, function);
    }
}

ExecutionUnit::ExecutionUnit(OBlockDesc oblock, vector<string> devices, vector<bool> isVtype, vector<string> controllers,
    TSQ<HeapMasterMessage> &sendMM, function<vector<BlsType>(vector<BlsType>)>  transform_function)
{
    this->Oblock = oblock;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
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

void ExecutionUnit::running(TSQ<HeapMasterMessage> &sendMM)
{
    while(true)
    {

        vector<HeapMasterMessage> currentHMMs = EUcache.read();
        std::unordered_map<DeviceID, HeapMasterMessage> HMMs;
        
        // Fill in the known data into the stack 
        for(auto &HMM : currentHMMs)
        {   
            HMMs[HMM.info.device] = HMM; 
        }
        
        vector<BlsType> transformableStates;

        for(auto& deviceDesc : this->Oblock.binded_devices){
            DeviceID devName = deviceDesc.device_name; 
            if(HMMs.contains(devName)){
                transformableStates.push_back(HMMs.at(devName).heapTree); 
            }
            else{
                auto defDevice = std::make_shared<MapDescriptor>(static_cast<TYPE>(deviceDesc.type), TYPE::string_t, TYPE::ANY); 
                transformableStates.push_back(defDevice); 
            }
        }
            
        transformableStates = transform_function(transformableStates);

        std::vector<HeapMasterMessage> outGoingStates;  

        // SHIP ALL OUTGOING DEVICES (EVEN ONCE NOT ALTERED)
        for(auto& devDesc : this->Oblock.binded_devices)
        {   
            int pos = this->devicePositionMap[devDesc.device_name]; 
            auto transformedState = std::get<shared_ptr<HeapDescriptor>>(transformableStates.at(pos));
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
    }
}

ExecutionUnit &ExecutionManager::assign(HeapMasterMessage DMM)
{   
    
    std::cout<<"Searching Oblock for: "<<DMM.info.oblock<<std::endl; 
    std::cout<<"Place into device: "<<DMM.info.device<<std::endl; 
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
        
        vector<HeapMasterMessage> currentHMMs = this->readMM.read();
       
        ExecutionUnit &assignedUnit = assign(currentHMMs.at(0));

        assignedUnit.EUcache.write(currentHMMs);
    
        if(assignedUnit.EUcache.getSize() < 3)
        {   
            std::cout<<"Requesting States"<<std::endl;
            O_Info info = assignedUnit.info;
            HeapMasterMessage requestHMM(nullptr, info, PROTOCOLS::REQUESTINGSTATES, false);
            this->sendMM.write(requestHMM);
        }
    
    }
}

