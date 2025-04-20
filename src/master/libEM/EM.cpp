#include "EM.hpp"
#include "include/Common.hpp"
#include "libtypes/bls_types.hpp"
#include <memory>

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, 
    TSQ<DynamicMasterMessage> &sendMM, GlobalContext& depMap)
    : readMM(readMM), sendMM(sendMM)
{
    this->OblockList = OblockList;

    
    for(auto& oblock : OblockList)
    {
        string OblockName = oblock.name;
        vector<string> devices;
        vector<bool> isVtype;
        vector<string> controllers;
        std::vector<DeviceDescriptor> devDesc; 
        int bytecodeOffset = oblock.bytecode_offset; 

        for(DeviceDescriptor& dev : oblock.binded_devices)
        {
            devDesc.push_back(dev); 
            devices.push_back(dev.device_name);
            isVtype.push_back(dev.isVtype);
            controllers.push_back(dev.controller);
        }
        EU_map[OblockName] = std::make_unique<ExecutionUnit>
        (OblockName, devices, isVtype, controllers, this->vtypeHMMsMap, this->sendMM, bytecodeOffset, devDesc);
    }
}

ExecutionUnit::ExecutionUnit(const string &OblockName, vector<string>& devices, vector<bool>& isVtype, vector<string>& controllers,
    TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, int bytecodeOffset, 
    vector<DeviceDescriptor> &devList)
{
    this->OblockName = OblockName;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
    this->info.oblock = OblockName;
    this->oblockOffset = bytecodeOffset; 


    //this->running(vtypeHMMsMap, sendMM);
    this->executionThread = thread(&ExecutionUnit::running, this, ref(vtypeHMMsMap), ref(sendMM));
    int i = 0; 
    for(auto& desc : devList){
        this->devDescs[desc.device_name] = desc; 
        this->devicePosMap[desc.device_name] = i;
        i++; 
    }
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
        vector<HeapMasterMessage> HMMs;


        for(int i = 0; i < currentDMMs.size(); i++)
        {   
            HeapMasterMessage HMM(currentDMMs.at(i).DM.toTree(), currentDMMs.at(i).info, 
            currentDMMs.at(i).protocol, currentDMMs.at(i).isInterrupt);
            HMMs.push_back(HMM);
        }

        vector<BlsType> transformableStates;
        transformableStates.reserve(this->devices.size());
        std::unordered_set<std::string> placedDevices; 


        // Fill in what we have from the Mailbox
        for(auto& hmm : HMMs)
        {   
            int pos = this->devicePosMap[hmm.info.device]; 
            transformableStates[pos] = hmm.heapTree;
            placedDevices.insert(hmm.info.device); 
        }

        // Find the remainder with the default constructors
        for(auto& dev : this->devices){
            if(!placedDevices.contains(dev)){
                auto &desc = this->devDescs[dev]; 

                if(desc.isVtype){
                    std::cout<<"Warning: Attempting to access a VTYPE (unimplemented)"<<std::endl; 
                }
                else{
                    auto device = std::make_shared<MapDescriptor>(static_cast<TYPE>(desc.devtype), TYPE::string_t, TYPE::ANY);
                    int pos = this->devicePosMap[dev]; 
                    transformableStates[pos] = device; 
                }
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

        this->vm.transform(this->oblockOffset, transformableStates); 

        for(int i = 0; i < transformableStates.size(); i++)
        {
            HMMs.at(i).heapTree = std::get<shared_ptr<HeapDescriptor>>(transformableStates.at(i));
        }

        vector<HeapMasterMessage> vtypeHMMs;
        for(auto& hmm : HMMs)
        {
            if(hmm.info.isVtype == false && (!hmm.heapTree->getAlteredAtr().empty()))
            {
                DynamicMessage DM; 
                DM.makeFromRoot(hmm.heapTree);        
                DynamicMasterMessage DMM(DM, hmm.info, 
                hmm.protocol, hmm.isInterrupt);
                sendMM.write(DMM);
            }
            else if(hmm.info.isVtype == true)
            {
                vtypeHMMs.push_back(hmm);
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

