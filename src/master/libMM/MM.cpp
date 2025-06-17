#include "MM.hpp"
#include "include/Common.hpp"
#include "libnetwork/Connection.hpp"
#include <exception>
#include <memory>
#include <unordered_set>

MasterMailbox::MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, 
    TSQ<DynamicMasterMessage> &readEM, TSQ<DynamicMasterMessage> &sendNM, TSQ<EMStateMessage> &sendEM)
: readNM(readNM), readEM(readEM), sendNM(sendNM), sendEM(sendEM)
{
    this->OBlockList = OBlockList;
    std::unordered_set<std::string> emplaced_set; 

    // Creating the read line
    for(auto &oblock : this->OBlockList)
    {
        oblockReadMap[oblock.name] = make_unique<ReaderBox>(oblock.dropRead, oblock.dropWrite, oblock.name, oblock);

        for(auto &devDesc : oblock.inDevices)
        {
            string deviceName = devDesc.device_name;
            auto TSQPtr = make_shared<TSQ<DynamicMasterMessage>>();
            auto& db =oblockReadMap[oblock.name]->waitingQs[deviceName];
            db.isTrigger = devDesc.isTrigger;
            db.stateQueues = TSQPtr; 
            db.devDropRead = devDesc.dropRead; 
            db.devDropWrite = devDesc.dropWrite; 
            db.deviceName = deviceName; 
        }

        // Write the out devics
        for(auto& devDesc : oblock.outDevices){
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
                    this->oblockReadMap[oblockName]->waitingQs[devName].lastMessage.replace(DMM); 
                    //std::cout<<"Wrote Callback into queue"<<std::endl; 
                    //this->oblockReadMap[oblock]->waitingQs[devName].stateQueues->write(DMM); 
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
                    targReadBox->insertState(DMM, this->sendEM); 
                    targReadBox->handleRequest(sendEM); 
                } 
            }
            else{
                OblockID targId = DMM.info.oblock; 
                if(!this->oblockReadMap.contains(targId)){break;}
                auto& rBox = this->oblockReadMap.at(targId); 
                rBox->insertState(DMM, sendEM);
                rBox->handleRequest(sendEM); 
            }
         
            break;
        }
        // Add handlers for any other stated
        case PROTOCOLS::OWNER_GRANT: {
            EMStateMessage ems; 
            ems.dmm_list = {DMM}; 
            ems.priority = -1; 
            ems.protocol = PROTOCOLS::OWNER_GRANT; 
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

void MasterMailbox::assignEM(DynamicMasterMessage DMM)
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
                    reader->insertState(DMM, this->sendEM); 
                }   
                break; 
            }

            WriterBox &assignedBox = *deviceWriteMap.at(DMM.info.device);
            bool dropWrite = correspondingReaderBox.dropWrite;

            if(dropWrite == true && assignedBox.waitingForCallback == true)
            {
                assignedBox.waitingQ.write(DMM);
            }
            else if(dropWrite == false && assignedBox.waitingForCallback == true)
            {
                assignedBox.waitingQ.write(DMM);
            }
            else if(dropWrite == false && assignedBox.waitingForCallback == false)
            {
                this->sendNM.write(DMM);
                assignedBox.waitingForCallback = true;
            }
            else if(dropWrite == true && assignedBox.waitingForCallback == false)
            {
                this->sendNM.write(DMM);
                assignedBox.waitingForCallback = true;
            }

            break;
        }
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST:{
            auto oblockName = DMM.info.oblock; 
            this->oblockReadMap[oblockName]->forwardPackets = true; 
            this->sendNM.write(DMM); 
            break; 
        }
        case PROTOCOLS::OWNER_CONFIRM: {
            // If confirms this means the oblock is not waiting for state and the readerbox can close
            auto oblockName = DMM.info.oblock; 
            this->oblockReadMap[oblockName]->forwardPackets = false; 
            this->sendNM.write(DMM); 
            break; 
        }
        case PROTOCOLS::OWNER_RELEASE:{
            auto oblockName = DMM.info.oblock; 
            this->oblockReadMap[oblockName]->forwardPackets = false; 
            this->sendNM.write(DMM); 
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
        DynamicMasterMessage currentDMM = this->readEM.read();
        assignEM(currentDMM);
    }
}
