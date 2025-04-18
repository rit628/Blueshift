#include "EM.hpp"
#include "include/Common.hpp"
#include <memory>

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, TSQ<DynamicMasterMessage> &sendMM)
    : readMM(readMM), sendMM(sendMM)
{
    this->OblockList = OblockList;
    for(auto& oDesc : this->OblockList)
    {
        string OblockName = oDesc.name;
        vector<string> devices;
        vector<bool> isVtype;
        vector<string> controllers;
        int oblockOffset = oDesc.bytecode_offset; 

        for(auto& bindedDev : oDesc.binded_devices)
        {
            devices.push_back(bindedDev.device_name);
            isVtype.push_back(bindedDev.isVtype);
            controllers.push_back(bindedDev.controller);
        }
        EU_map[OblockName] = std::make_unique<ExecutionUnit>(OblockName, devices, isVtype, controllers, 
        this->vtypeHMMsMap, this->sendMM, oblockOffset);
    }
}

ExecutionUnit::ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers,
    TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, int oblockOffset)
{
    this->OblockName = OblockName;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
    this->bytecodeOffset = oblockOffset; 
    this->info.oblock = OblockName;

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
    while(1)
    {
        if(EUcache.isEmpty()) {continue;}
        vector<DynamicMasterMessage> currentDMMs = EUcache.read();
        vector<HeapMasterMessage> HMMs;


        for(int i = 0; i < currentDMMs.size(); i++)
        {   
            HeapMasterMessage HMM(currentDMMs.at(i).DM.toTree(), currentDMMs.at(i).info, 
            currentDMMs.at(i).protocol, currentDMMs.at(i).isInterrupt);
            HMMs.push_back(HMM);
        }

        vector<BlsType> transformableStates;
        for(int i = 0; i < HMMs.size(); i++)
        {   
            transformableStates.push_back(HMMs.at(i).heapTree);
        }
        auto result = vtypeHMMsMap.get(HMMs.at(0).info.oblock);
        if(result.has_value())
        {
            for(int i = 0; i < result->size(); i++)
            {
                transformableStates.push_back(result->at(i).heapTree);
            }
        }

        // TODO: Populate written to transformed states here: 
    
        this->vm.transform(this->bytecodeOffset, transformableStates); 

        // TODO: Extract the written states here: 

        for(int i = 0; i < transformableStates.size(); i++)
        {
            HMMs.at(i).heapTree = std::get<shared_ptr<HeapDescriptor>>(transformableStates.at(i));
        }

        vector<HeapMasterMessage> vtypeHMMs;
        for(int i = 0; i < HMMs.size(); i++)
        {
            if(HMMs.at(i).info.isVtype == false)
            {
                DynamicMessage DM; 
                DM.makeFromRoot(HMMs.at(i).heapTree);        
                DynamicMasterMessage DMM(DM, HMMs.at(i).info, 
                HMMs.at(i).protocol, HMMs.at(i).isInterrupt);
                sendMM.write(DMM);
            }
            else if(HMMs.at(i).info.isVtype == true)
            {
                vtypeHMMs.push_back(HMMs.at(i));
                //vtypeHMMsMap.insert(HMMs.at(i).info.oblock, HMMs.at(i));
            }
        }
        if(!vtypeHMMs.empty())
        {
            vtypeHMMsMap.insert(HMMs.at(0).info.oblock, vtypeHMMs);
        }
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

