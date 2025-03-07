#include "EM.hpp"
#include <memory>

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, 
    TSQ<DynamicMasterMessage> &sendMM, 
    std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks)
    : readMM(readMM), sendMM(sendMM)
{
    this->OblockList = OblockList;
    for(int i = 0; i < OblockList.size(); i++)
    {
        string OblockName = OblockList.at(i).name;
        vector<string> devices;
        vector<bool> isVtype;
        vector<string> controllers;
        for(int j = 0; j < OblockList.at(i).binded_devices.size(); j++)
        {
            devices.push_back(OblockList.at(i).binded_devices.at(j).device_name);
            isVtype.push_back(OblockList.at(i).binded_devices.at(j).isVtype);
            controllers.push_back(OblockList.at(i).binded_devices.at(j).controller);
        }
        auto function = oblocks[OblockName];
        EU_map[OblockName] = std::make_unique<ExecutionUnit>
        (OblockName, devices, isVtype, controllers, this->vtypeHMMsMap, this->sendMM, function);
    }
}

ExecutionUnit::ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers,
    TSM<string, HeapMasterMessage> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, 
    function<vector<BlsLang::BlsType>(vector<BlsLang::BlsType>)>  transform_function)
{
    this->OblockName = OblockName;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
    this->transform_function = transform_function;

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

void ExecutionUnit::running(TSM<string, HeapMasterMessage> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM)
{
    while(1)
    {
        if(EUcache.isEmpty()) {continue;}
        vector<DynamicMasterMessage> currentDMMs = EUcache.read();
        std::cout<<"This: "<<this->OblockName<<std::endl;
        vector<HeapMasterMessage> HMMs;


        for(int i = 0; i < currentDMMs.size(); i++)
        {   
            HeapMasterMessage HMM(currentDMMs.at(i).DM.toTree(), currentDMMs.at(i).info, 
            currentDMMs.at(i).protocol, currentDMMs.at(i).isInterrupt);
            HMMs.push_back(HMM);
        }

        vector<BlsLang::BlsType> transformableStates;
        for(int i = 0; i < HMMs.size(); i++)
        {   
            transformableStates.push_back(HMMs.at(i).heapTree);
        }
    
        transformableStates = transform_function(transformableStates);

        for(int i = 0; i < transformableStates.size(); i++)
        {
            HMMs.at(i).heapTree = std::get<shared_ptr<HeapDescriptor>>(transformableStates.at(i));
        }

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
                vtypeHMMsMap.insert(HMMs.at(i).info.oblock, HMMs.at(i));
            }
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
    while(1)
    {
        
        vector<DynamicMasterMessage> currentDMMs = this->readMM.read();
       
        ExecutionUnit &assignedUnit = assign(currentDMMs.at(0));

        assignedUnit.EUcache.write(currentDMMs);
        /*
        if(assignedUnit.EUcache.getSize() < 3)
        {   
            DynamicMessage dm;
            O_Info info = assignedUnit.info;
            DynamicMasterMessage requestDMM(dm, info, PROTOCOLS::REQUESTINGSTATES, false);
            this->sendMM.write(requestDMM);
        }
        */ 
    }
}

