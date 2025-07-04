#include "MM.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtype/bls_types.hpp"
#include <exception>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <variant>

MasterMailbox::MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, 
    TSQ<HeapMasterMessage> &readEM, TSQ<DynamicMasterMessage> &sendNM, TSQ<vector<HeapMasterMessage>> &sendEM)
: readNM(readNM), readEM(readEM), sendNM(sendNM), sendEM(sendEM)
{
    this->OBlockList = OBlockList;
    std::unordered_set<std::string> emplaced_set; 

    // Creating the read line
    for(auto &oblock : this->OBlockList)
    {
        oblockReadMap[oblock.name] = make_unique<ReaderBox>(oblock.dropRead, oblock.dropWrite, oblock.name, oblock);

        for(auto &devDesc : oblock.binded_devices)
        { 
            string deviceName = devDesc.device_name;
            auto TSQPtr = make_shared<TSQ<HeapMasterMessage>>();
            auto& db =oblockReadMap[oblock.name]->waitingQs[deviceName];
            db.isTrigger = devDesc.isTrigger;
            db.stateQueues = TSQPtr; 
            db.devDropRead = devDesc.dropRead; 
            db.devDropWrite = devDesc.dropWrite; 
            db.deviceName = deviceName; 

            // Generate the new state
            if(devDesc.isVtype){
                std::cout<<"This is for a vtype"<<std::endl; 
                auto defDevice = std::make_shared<MapDescriptor>(static_cast<TYPE>(devDesc.type), TYPE::string_t, TYPE::ANY); 
                HeapMasterMessage hmm;
                hmm.heapTree = defDevice; 
                hmm.info.isVtype = true; 
                hmm.info.device = devDesc.device_name; 
                hmm.info.controller = "MASTER"; 
                oblockReadMap[oblock.name]->insertState(hmm); 
            } 
        }




        // Write the out devics
        for(auto& devDesc : oblock.binded_devices){
            if(!emplaced_set.contains(devDesc.device_name)){
                auto wb = std::make_unique<WriterBox>();
                wb->deviceName = devDesc.device_name; 
                wb->waitingForCallback = false; 
                this->deviceWriteMap[devDesc.device_name] = std::move(wb); 
                emplaced_set.insert(devDesc.device_name); 
            }
        }
    }


    // populate the device to oblock_set mapping for interrupt devices (as state not copied by Timer system)
    for(auto &oblock : OBlockList)
    {
        for(auto &devDesc : oblock.binded_devices){
            string deviceName = devDesc.device_name;
            interruptName_map[deviceName].push_back(oblock.name);   
        }
    }
}

ReaderBox::ReaderBox(bool dropRead, bool dropWrite, string name, OBlockDesc& oDesc)
: triggerMan(oDesc)
{
    this->dropRead = dropRead;
    this->dropWrite = dropWrite;
    this->OblockName = name;
}

WriterBox::WriterBox(string deviceName)
{
    this->deviceName = deviceName;
}

void MasterMailbox::assignNM(DynamicMasterMessage DMM)
{

    switch(DMM.protocol)
    {
        case PROTOCOLS::CALLBACKRECIEVED:
        {
            DeviceID devName = DMM.info.device; 

            // For now we count callbacks to update the state in the mailbox (CHECK IF CALLBACK DEV IN READ LIST)
            for(auto &oblockName : this->interruptName_map[devName]){
                if(this->oblockReadMap[oblockName]->waitingQs.contains(devName)){
                    DMM.info.oblock = oblockName; 
                    // Conversion of DMM callback to HMM
                    HeapMasterMessage hmm; 
                    hmm.protocol = PROTOCOLS::CALLBACKRECIEVED; 
                    hmm.info = DMM.info; 
                    hmm.heapTree = DMM.DM.toTree(); 
                    this->oblockReadMap[oblockName]->waitingQs[devName].lastMessage.replace(hmm); 
                }
            }
           
            if(!deviceWriteMap.at(DMM.info.device)->waitingQ.isEmpty())
            {
                DynamicMasterMessage DMMtoSend = deviceWriteMap.at(DMM.info.device)->waitingQ.read();
                this->sendNM.write(DMMtoSend);
            }
            else
            {
                deviceWriteMap.at(DMM.info.device)->waitingForCallback = false;
            }
            break;
        }
        case PROTOCOLS::SENDSTATES:
        {
            if(DMM.isInterrupt){
                std::vector<OblockID> oList = this->interruptName_map[DMM.info.device];
                for(auto& oid : oList){
                    if(!this->oblockReadMap.contains(oid)){break;}
                    auto& targReadBox = this->oblockReadMap[oid]; 
                    DMM.info.oblock = oid; 

                    HeapMasterMessage newHMM(DMM); 
                    targReadBox->insertState(newHMM); 
                    targReadBox->handleRequest(sendEM); 
                } 
            }
            else{
                OblockID targId = DMM.info.oblock; 
                if(!this->oblockReadMap.contains(targId)){break;}
                auto& rBox = this->oblockReadMap.at(targId); 
                rBox->insertState(DMM);
                rBox->handleRequest(sendEM); 
            }
         
            break;
        }
        default:
        {
            break;
        }
    }
}

void MasterMailbox::assignEM(HeapMasterMessage DMM)
{
    ReaderBox &correspondingReaderBox = *oblockReadMap.at(DMM.info.oblock);
    switch(DMM.protocol)
    {
        case PROTOCOLS::REQUESTINGSTATES:
        {
            correspondingReaderBox.pending_requests = true; 
            break; 
        }
        case PROTOCOLS::SENDSTATES:
        {
            if(DMM.info.isVtype){
                // Notify the relevant devices (store into the slots for the master devices)
                std::vector<OblockID> oblockList = this->interruptName_map[DMM.info.device]; 
                for(auto &name : oblockList){
                    std::unique_ptr<ReaderBox>& reader = this->oblockReadMap[name]; 
                    std::cout<<"Inserting vtype into state:"<<std::endl; 
                    
                    reader->insertState(DMM); 
                    reader->handleRequest(this->sendEM);
                }   
                break; 
            }

            WriterBox &assignedBox = *deviceWriteMap.at(DMM.info.device);
            bool dropWrite = correspondingReaderBox.dropWrite;

            DynamicMasterMessage realDMM;
            realDMM.info = DMM.info;  
            realDMM.protocol = PROTOCOLS::SENDSTATES; 
            realDMM.isInterrupt = DMM.isInterrupt; 

            DynamicMessage dm; 
            if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(DMM.heapTree)){
                dm.makeFromRoot(std::get<std::shared_ptr<HeapDescriptor>>(DMM.heapTree)); 
            }
            else{
                throw std::invalid_argument("Attempting to serialize Non-Heap Descriptor state for state sending!"); 
            }

            realDMM.DM = dm; 
            
            if(dropWrite == true && assignedBox.waitingForCallback == true)
            {
                assignedBox.waitingQ.write(realDMM);
            }
            else if(dropWrite == false && assignedBox.waitingForCallback == true)
            {
                assignedBox.waitingQ.write(realDMM);
            }
            else if(dropWrite == false && assignedBox.waitingForCallback == false)
            {
                this->sendNM.write(realDMM);
                assignedBox.waitingForCallback = true;
            }
            else if(dropWrite == true && assignedBox.waitingForCallback == false)
            {
                this->sendNM.write(realDMM);
                assignedBox.waitingForCallback = true;
            }

            break;
        }
        default:
        {
            break;
        }
    }
}

void MasterMailbox::runningNM()
{
    while(true)
    {
        DynamicMasterMessage currentDMM = this->readNM.read();  
        assignNM(currentDMM);
    }
}

void MasterMailbox::runningEM()
{
    while(true){
        HeapMasterMessage currentDMM = this->readEM.read();
        assignEM(currentDMM);
    }
}
