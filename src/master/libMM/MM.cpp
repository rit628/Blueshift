#include "MM.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include "libtype/bls_types.hpp"
#include <exception>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <variant>


MasterMailbox::MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, 
    TSQ<HeapMasterMessage> &readEM, TSQ<DynamicMasterMessage> &sendNM, TSQ<EMStateMessage> &sendEM)
: readNM(readNM), readEM(readEM), sendNM(sendNM), sendEM(sendEM)
{
    this->OBlockList = OBlockList;
    std::unordered_set<std::string> emplaced_set; 

    // Creating the read line
    for(auto &oblock : this->OBlockList)
    {
        oblockReadMap[oblock.name] = make_unique<ReaderBox>(oblock.name, oblock, this->triggerSet);

        for(auto &devDesc : oblock.binded_devices)
        { 
            string deviceName = devDesc.device_name;
            auto TSQPtr = make_shared<TSQ<HeapMasterMessage>>();
            auto& db =oblockReadMap[oblock.name]->waitingQs[deviceName];
            db.stateQueues = TSQPtr; 
            db.devDropRead = devDesc.dropRead; 
            db.devDropWrite = devDesc.dropWrite; 
            db.deviceName = deviceName; 

            // Generate the new state
            if(devDesc.isVtype){
                std::cout<<"This is for a vtype"<<std::endl; 
                HeapMasterMessage hmm;

                if(oblock.name == "task2"){
                    std::cout<<std::endl; 
                }

                hmm.heapTree = devDesc.initialValue; 
                hmm.info.isVtype = true; 
                hmm.info.device = devDesc.device_name; 
                hmm.info.controller = "MASTER"; 
                oblockReadMap[oblock.name]->insertState(hmm, this->sendEM); 
            } 
        }

        // Write the out devics
        for(auto& devDesc : oblock.binded_devices){
            if(!emplaced_set.contains(devDesc.device_name)){
                auto wb = std::make_unique<WriterBox>();
                wb->deviceName = devDesc.device_name; 
                wb->waitingForCallback = false; 
                this->parentCont[devDesc.device_name] = devDesc.controller; 
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

DynamicMasterMessage MasterMailbox::buildDMM(HeapMasterMessage &hmm){
    DynamicMasterMessage dmm; 
    dmm.info = hmm.info; 
    dmm.isInterrupt = hmm.isInterrupt; 
    dmm.protocol = hmm.protocol; 
    if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(hmm.heapTree)){
        auto omar = std::get<std::shared_ptr<HeapDescriptor>>(hmm.heapTree); 
        dmm.DM.makeFromRoot(omar); 
    }
  
    return dmm;
}

ReaderBox::ReaderBox(string name, OBlockDesc& oDesc, std::unordered_set<OblockID> &triggerSet)
: triggerSet(triggerSet), triggerMan(oDesc)
{
    this->OblockName = name;
    this->oblockDesc = oDesc; 
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
                    this->oblockReadMap[oblockName]->insertState(hmm, this->sendEM); 
                }
            }

            std::cout<<"UPDATING CALLBACKS FOR TRIGGERS"<<std::endl; 
            for(auto& oblockName : this->interruptName_map[devName]){
                this->oblockReadMap[oblockName]->handleRequest(this->sendEM); 
            }
           
            // Send the next device for items waiting for a callback 
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
                    targReadBox->insertState(newHMM, this->sendEM); 
                } 
                std::cout<<"Oblocks Triggered: "<<this->triggerSet.size()<<std::endl; 

                for(auto& oid : oList){
                    if(!this->oblockReadMap.contains(oid)){break;}
                    auto& targReadBox = this->oblockReadMap[oid]; 
                    targReadBox->handleRequest(this->sendEM); 
                }
            }
            else{
                OblockID targId = DMM.info.oblock; 
                if(!this->oblockReadMap.contains(targId)){break;}
                auto& rBox = this->oblockReadMap.at(targId); 
                rBox->insertState(DMM, this->sendEM);
                rBox->handleRequest(sendEM); 
            }
         
            break;
        }
        // Add handlers for any other stated
        case PROTOCOLS::OWNER_CONFIRM_OK: 
        case PROTOCOLS::OWNER_GRANT: {

            if(DMM.protocol == PROTOCOLS::OWNER_CONFIRM_OK){
                std::cout<<"Mailbox: Received Owner confirmation "<<DMM.info.oblock<<" for device "<<DMM.info.device<<std::endl; 
            }
            else{
                std::cout<<"Mailbox: Grant received for oblock "<<DMM.info.oblock<<" for device: "<<DMM.info.device<<std::endl; 
            }
            
            EMStateMessage ems; 
            ems.dmm_list = {DMM}; 
            ems.priority = -1; 
            ems.protocol = DMM.protocol; 
            ems.TriggerName = ""; 
            ems.oblockName = DMM.info.oblock; 
            this->sendEM.write(ems); 
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
                     if(!this->oblockReadMap.contains(name)){break;}
                    std::unique_ptr<ReaderBox>& reader = this->oblockReadMap[name]; 
                    reader->insertState(DMM, this->sendEM); 
                }   
                std::cout<<"Trigger set size: "<<triggerSet.size()<<std::endl; 
                for(auto& trig : triggerSet){
                    std::cout<<trig<<std::endl; 
                }


                for(auto& name : oblockList){
                    if(!this->oblockReadMap.contains(name)){break;}
                    std::unique_ptr<ReaderBox>& reader = this->oblockReadMap[name]; 
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
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST:{   
            auto oblockName = DMM.info.oblock; 
            std::cout<<"Mailbox Ownership request for the device: "<<DMM.info.device<<" from oblock "<<oblockName<<std::endl; 
            this->oblockReadMap[oblockName]->forwardPackets = true; 
            this->targetedDevices.insert(DMM.info.device); 
            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
            break; 
        }
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE:{
            std::cout<<"What!"<<std::endl;
            auto oblockName = DMM.info.oblock; 
            if(this->triggerSet.contains(oblockName)){
                if(++requestCount == this->triggerSet.size()){
                    // Loop target devices: 
                    for(auto& dev : targetedDevices){
                        DynamicMasterMessage dmm; 
                        dmm.protocol = PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE; 
                        dmm.info.controller = this->parentCont.at(dev); 
                        dmm.info.device = dev; 
                        this->sendNM.write(dmm); 
                    }
                    requestCount = 0; 
                    this->triggerSet.clear(); 
                    targetedDevices.clear(); 
                }
            }
            break; 
        }
        case PROTOCOLS::OWNER_CONFIRM: {
            // If confirms this means the oblock is not waiting for state and the readerbox can close
            auto oblockName = DMM.info.oblock; 
            std::cout<<"Mailbox Owner Confirmation for the device: "<<DMM.info.device<<" from oblock "<<oblockName<<std::endl; 
            this->oblockReadMap[oblockName]->forwardPackets = false; 
            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
            break; 
        }
        case PROTOCOLS::OWNER_RELEASE:{
            auto oblockName = DMM.info.oblock; 
            std::cout<<"Mailbox Owner Release for the device: "<<DMM.info.device<<" from oblock "<<oblockName<<std::endl; 
            this->oblockReadMap[oblockName]->forwardPackets = false; 
            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
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
        std::cout<<"Recieved from NM: "<<currentDMM.info.device<<std::endl; 
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
