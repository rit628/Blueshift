#include "EM.hpp"
#include "include/Common.hpp"
#include "libMM/MM.hpp"
#include "libtypes/bls_types.hpp"
#include <memory>

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, 
    TSQ<DynamicMasterMessage> &sendMM, 
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
        EU_map[OblockName] = std::make_unique<ExecutionUnit>
        (oblock, devices, isVtype, controllers, this->vtypeHMMsMap, this->sendMM, function);
    }
}

ExecutionUnit::ExecutionUnit(OBlockDesc oblock, vector<string> devices, vector<bool> isVtype, vector<string> controllers,
    TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, 
    function<vector<BlsType>(vector<BlsType>)>  transform_function)
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
    this->executionThread = thread(&ExecutionUnit::running, this, ref(vtypeHMMsMap), ref(sendMM));
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

void ExecutionUnit::running(TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM)
{
    while(true)
    {
        if(EUcache.isEmpty()) {continue;}
        vector<DynamicMasterMessage> currentDMMs = EUcache.read();
        std::unordered_map<DeviceID, HeapMasterMessage> HMMs;
        
        // Fill in the known data into the stack 
        for(auto &dmm : currentDMMs)
        {   
            HeapMasterMessage HMM(dmm.DM.toTree(), dmm.info, 
            dmm.protocol, dmm.isInterrupt);

            HMMs[HMM.info.device] = HMM; 
        }
        
        vector<BlsType> transformableStates;

        for(int i = 0; i < this->Oblock.binded_devices.size(); i++){
            auto& deviceDesc = this->Oblock.binded_devices[i];
            DeviceID devName = deviceDesc.device_name; 
            // ADD VTYPE SUPPORT HERE AS WELL
            if(HMMs.contains(devName)){
                transformableStates.push_back(HMMs.at(devName).heapTree); 
            }
            else{
                auto defDevice = std::make_shared<MapDescriptor>(static_cast<TYPE>(deviceDesc.devtype), TYPE::string_t, TYPE::ANY); 
                std::cout<<"devDesc Name: "<<deviceDesc.device_name<<std::endl; 
                transformableStates.push_back(defDevice); 
            }
        }
        

        /*
        auto result = vtypeHMMsMap.get(HMMs.at(0).info.oblock);

        if(result.has_value())
        {
            for(int i = 0; i < result->size(); i++)
            {
                transformableStates.push_back(result->at(i).heapTree);
            }
        }
        */ 
    
        transformableStates = transform_function(transformableStates);

        std::vector<HeapMasterMessage> outGoingStates;  

        std::cout<<"Outgoing states"<<std::endl;

        // SHIP ALL OUTGOING DEVICES (EVEN ONCE NOT ALTERED)
        for(auto& devDesc : this->Oblock.outDevices)
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

        std::cout<<"Print 2"<<std::endl; 

        vector<HeapMasterMessage> vtypeHMMs;
        for(HeapMasterMessage& hmm : outGoingStates)
        {
            if(hmm.info.isVtype == false)
            {
                DynamicMessage DM; 
                std::cout<<"Precending"<<std::endl; 
                DM.makeFromRoot(hmm.heapTree);    
                DynamicMasterMessage DMM(DM, hmm.info, 
                hmm.protocol, hmm.isInterrupt);
                std::cout<<"Incoming"<<std::endl; 
                sendMM.write(DMM);
            }
            else if(hmm.info.isVtype == true)
            {
                vtypeHMMs.push_back(hmm);
                //vtypeHMMsMap.insert(HMMs.at(i).info.oblock, HMMs.at(i));
            }
        }
        /*
        if(!vtypeHMMs.empty())
        {
            vtypeHMMsMap.insert(HMMs.at(0).info.oblock, vtypeHMMs);
        }
        */ 
    }
}

ExecutionUnit &ExecutionManager::assign(DynamicMasterMessage DMM)
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
                DynamicMessage dm;
                DynamicMasterMessage requestDMM(dm, info, PROTOCOLS::REQUESTINGSTATES, false);
                this->sendMM.write(requestDMM);

            }
            started = false; 
        }
        
        vector<DynamicMasterMessage> currentDMMs = this->readMM.read();

        std::cout<<"Recieved States"<<std::endl;
       
        ExecutionUnit &assignedUnit = assign(currentDMMs.at(0));

        assignedUnit.EUcache.write(currentDMMs);
    
        if(assignedUnit.EUcache.getSize() < 3)
        {   
            std::cout<<"Requesting States"<<std::endl;
            DynamicMessage dm;
            O_Info info = assignedUnit.info;
            DynamicMasterMessage requestDMM(dm, info, PROTOCOLS::REQUESTINGSTATES, false);
            this->sendMM.write(requestDMM);
        }
    
    }
}

