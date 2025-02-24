#include "EM.hpp"
#include <memory>
#include <vector>

ExecutionManager::ExecutionManager(vector<OBlockDesc> OblockList, TSQ<DynamicMasterMessage> &in, TSQ<HeapMasterMessage> &out)
    : in(in), out(out)
{
    this->OblockList = OblockList;
    vector<string> devices;
    vector<bool> isVtype;
    vector<string> controllers;
    for(int i = 0; i < OblockList.size(); i++)
    {
        string OblockName = OblockList.at(i).name;
        for(int j = 0; j < OblockList.at(i).binded_devices.size(); j++)
        {
            devices.push_back(OblockList.at(i).binded_devices.at(j).device_name);
            isVtype.push_back(OblockList.at(i).binded_devices.at(j).isVtype);
            controllers.push_back(OblockList.at(i).binded_devices.at(j).controller);
        }
        ExecutionUnit tempUnit = ExecutionUnit(OblockName, devices, isVtype, controllers);
        for(int j = 0; j < OblockList.at(i).binded_devices.size(); j++)
        {
            EU_map.emplace(OblockName, tempUnit);
        }
    }
}

ExecutionUnit::ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers)
{
    this->OblockName = OblockName;
    this->devices = devices;
    this->isVtype = isVtype;
    this->controllers = controllers;
}

HeapMasterMessage::HeapMasterMessage(shared_ptr<HeapDescriptor> heapTree, O_Info info)
{
    this->heapTree = heapTree;
    this->info = info;
}

vector<HeapMasterMessage> ExecutionUnit::process(DynamicMasterMessage DMM, ExecutionUnit unit)
{
    vector<HeapMasterMessage> transformableMessages;
    for(auto it = stateMap.begin(); it != stateMap.end(); ++it) 
    {
        //HeapMasterMessage *HMM = new HeapMasterMessage(it->second.DM.toTree(), DMM.info);
        HeapMasterMessage HMM(it->second.DM.toTree(), DMM.info);
        transformableMessages.push_back(HMM);
    }
    return transformableMessages;
}

ExecutionUnit &ExecutionManager::assign(DynamicMasterMessage DMM)
{
    ExecutionUnit &assignedUnit = EU_map.at(DMM.info.oblock);
    assignedUnit.stateMap.emplace(DMM.info.device, DMM);
    return assignedUnit;
}

vector<shared_ptr<HeapDescriptor>> ExecutionManager::transformState(vector<shared_ptr<HeapDescriptor>> HMM_List)
{
    cout << "State transformed" << endl;
    return HMM_List;
}

void ExecutionManager::running(TSQ<DynamicMasterMessage> &in)
{
    //This while condition is temporary as we need to have a kill condition or something when it is no longer
    //running
    while(!in.isEmpty())
    {
        DynamicMasterMessage currentDMM = in.read();
        ExecutionUnit assignedUnit = assign(currentDMM);
        vector<HeapMasterMessage> HMMs = assignedUnit.process(currentDMM, assignedUnit);
        vector<shared_ptr<HeapDescriptor>> transformedStates;
        for(int i = 0; i < HMMs.size(); i++)
        {
            transformedStates.push_back(HMMs.at(i).heapTree); 
        }
        transformedStates = transformState(transformedStates);
        for(int i = 0; i < HMMs.size(); i++)
        {
            HMMs.at(i).heapTree = transformedStates.at(i);
        }
        for(int i = 0; i < HMMs.size(); i++)
        {
            if(HMMs.at(i).info.isVtype == true)
            {
                this->vtypeHMMs.push_back(HMMs.at(i));
                continue;
            }
            else
            {
                out.write(HMMs.at(i));
            }
        }
    }
}

