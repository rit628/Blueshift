#include "MM.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include "libTSQ/TSQ.hpp"
#include "libtype/bls_types.hpp"
#include <exception>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <variant>


MasterMailbox::MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, 
    TSQ<HeapMasterMessage> &readEM, TSQ<DynamicMasterMessage> &sendNM, TSQ<EMStateMessage> &sendEM)
: readNM(readNM), readEM(readEM), sendEM(sendEM), sendNM(sendNM),  ConfContainer(sendNM ,OBlockList)
{
    this->OBlockList = OBlockList;
    std::unordered_set<std::string> emplaced_set; 
    

    // Creating the read line
    for(auto &oblock : this->OBlockList)
    {
        oblockReadMap[oblock.name] = make_unique<ReaderBox>(oblock.name, oblock, this->triggerSet, sendEM);

        for(auto &devDesc : oblock.binded_devices)
        { 
            string deviceName = devDesc.device_name;
            auto TSQPtr = make_shared<TSQ<HeapMasterMessage>>();
            auto& db =oblockReadMap[oblock.name]->waitingQs[deviceName];
            db.stateQueues = TSQPtr; 
            db.readPolicy = devDesc.readPolicy;
            db.overwritePolicy = devDesc.overwritePolicy; 
            db.yields = devDesc.isYield;
            db.deviceName = deviceName; 

            // Generate the new state
            if(devDesc.isVtype){
                //std::cout<<"This is for a vtype"<<std::endl; 
                HeapMasterMessage hmm;
                hmm.heapTree = devDesc.initialValue; 
                hmm.info.isVtype = true; 
                hmm.info.device = devDesc.device_name; 
                hmm.info.controller = "MASTER"; 
                oblockReadMap[oblock.name]->insertState(hmm); 
            } 
        }

        // Write the out devics
        for(auto& devDesc : oblock.binded_devices){
            DeviceID devName = devDesc.device_name;
            if(devDesc.deviceKind == DeviceKind::CURSOR){
                devName = devName + "::" + oblock.name; 
            } 
            if(!emplaced_set.contains(devDesc.device_name)){
                auto wb = std::make_unique<WriterBox>(devDesc,  sendNM, this->ConfContainer);
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
            if(devDesc.isVtype){
                this->vTypesSchedule.emplace(devDesc.device_name, ManagedVType{}); 
            }
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

ReaderBox::ReaderBox(string name, OBlockDesc& oDesc, std::unordered_set<OblockID> &triggerSet, TSQ<EMStateMessage> &ems)
: triggerSet(triggerSet), triggerMan(oDesc), sendEM(ems)
{
    this->OblockName = name;
    this->oblockDesc = oDesc; 

    for(auto& desc : oDesc.binded_devices){
        this->devDesc.emplace(desc.device_name, desc); 
    }
}


void MasterMailbox::assignNM(DynamicMasterMessage DMM)
{

    switch(DMM.protocol)
    {   
        case PROTOCOLS::CALLBACKRECIEVED:
        {   
            DeviceID devName = DMM.info.device; 


        if(!DMM.isCursor){
            // For now we count callbacks to update the state in the mailbox (CHECK IF CALLBACK DEV IN READ LIST)
            for(auto &oblockName : this->interruptName_map[devName]){
                if(this->oblockReadMap.at(oblockName)->waitingQs.contains(devName)){  
                    DMM.info.oblock = oblockName; 
                    HeapMasterMessage hmm; 
                    hmm.protocol = PROTOCOLS::CALLBACKRECIEVED; 
                    hmm.info = DMM.info; 
                    hmm.heapTree = DMM.DM.toTree(); 
                    this->oblockReadMap[oblockName]->insertState(hmm); 
                }
            }
            for(auto& oblockName : this->interruptName_map[devName]){
                this->oblockReadMap[oblockName]->handleRequest(); 
            } 
        }
        else{
            auto& readMap = this->oblockReadMap.at(DMM.info.oblock); 
            readMap->insertState(DMM); 
            readMap->handleRequest(); 
        }  

        if(DMM.isCursor){
            devName = devName + "::" + DMM.info.oblock; 
        }
        deviceWriteMap.at(devName)->notifyCallBack(); 
        
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
                } 
                //std::cout<<"Oblocks Triggered: "<<this->triggerSet.size()<<std::endl; 

                for(auto& oid : oList){
                    if(!this->oblockReadMap.contains(oid)){break;}
                    auto& targReadBox = this->oblockReadMap[oid]; 
                    targReadBox->handleRequest(); 
                }
            }
            else{
                OblockID targId = DMM.info.oblock; 
                if(!this->oblockReadMap.contains(targId)){break;}
                auto& rBox = this->oblockReadMap.at(targId); 
                rBox->insertState(DMM);
                rBox->handleRequest(); 
            }
         
            break;
        }
        // Add handlers for any other stated
        case PROTOCOLS::OWNER_CONFIRM_OK: 
        case PROTOCOLS::OWNER_GRANT: {
            EMStateMessage ems; 
            ems.dmm_list = {DMM}; 
            ems.priority = -1; 
            ems.protocol = DMM.protocol; 
            ems.TriggerName = ""; 
            ems.oblockName = DMM.info.oblock; 
            this->sendEM.write(ems); 
            break; 
        }
        case PROTOCOLS::PULL_RESPONSE:{
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
        case PROTOCOLS::PUSH_REQUEST: 
        case PROTOCOLS::SENDSTATES:

        {
            DeviceID dev = DMM.info.device; 
            if(DMM.info.isVtype){
                // Notify the relevant devices (store into the slots for the master devices)
                if(this->vTypesSchedule.at(DMM.info.device).owner != DMM.info.oblock){
                    std::cout<<"Write attempted by unowned oblock"<<std::endl; 
                    break; 
                }

                std::vector<OblockID> oblockList = this->interruptName_map.at(DMM.info.device); 
                for(auto &name : oblockList){
                    if(!this->oblockReadMap.contains(name)){break;}
                    DMM.protocol = PROTOCOLS::CALLBACKRECIEVED; 
                    std::unique_ptr<ReaderBox>& reader = this->oblockReadMap[name]; 
                    reader->insertState(DMM); 
                }   
           
                for(auto& name : oblockList){
                    if(!this->oblockReadMap.contains(name)){break;}
                    std::unique_ptr<ReaderBox>& reader = this->oblockReadMap[name]; 
                    reader->handleRequest();
                }
              
                break; 
            }

            if(DMM.isCursor){
                dev = dev + "::" + DMM.info.oblock; 
            }

            WriterBox &assignedBox = *deviceWriteMap.at(dev);
            auto owPolicy = oblockReadMap.at(DMM.info.oblock)->waitingQs.at(DMM.info.device).overwritePolicy;

            assignedBox.writeOut(DMM, owPolicy, PROTOCOLS::SENDSTATES); 
            break;
        }
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST:{   
            auto oblockName = DMM.info.oblock; 
            //std::cout<<"Mailbox Ownership request for the device: "<<DMM.info.device<<" from oblock "<<oblockName<<std::endl; 
            this->oblockReadMap[oblockName]->forwardPackets = true; 
            this->targetedDevices.insert(DMM.info.device); 

            if(this->vTypesSchedule.contains(DMM.info.device)){
                //std::cout<<"Vtype candidate request for: "<<DMM.info.device<<" from oblock "<<DMM.info.oblock<<std::endl; 
                SchedulerReq req; 
                req.requestorOblock = DMM.info.oblock;
                req.targetDevice = DMM.info.device; 
                this->vTypesSchedule.at(DMM.info.device).queue.getQueue().push(req); 
                break; 
            }

            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
            break; 
        }
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE:{
        
            auto oblockName = DMM.info.oblock; 
            if(this->triggerSet.contains(oblockName)){
                this->triggerSet.erase(oblockName); 
            }

            if(triggerSet.empty()){
                for(auto& dev : targetedDevices){
                    if(this->vTypesSchedule.contains(dev)){
                        auto& schedule = this->vTypesSchedule.at(dev);
                        auto request = schedule.queue.getQueue().top();
                        schedule.owner = request.requestorOblock; 
                        schedule.isOwned = false; 
                        OblockID targetOblock = request.requestorOblock;
                        //std::cout<<"Sending vtype grant for oblock "<<targetOblock<<" for vtype "<<dev<<std::endl;
                        EMStateMessage ems; 
                        HeapMasterMessage hmm;
                        hmm.protocol = PROTOCOLS::OWNER_GRANT; 
                        hmm.info.device = dev; 
                        hmm.info.oblock = targetOblock;  
                        ems.dmm_list = {hmm}; 
                        ems.priority = -1; 
                        ems.protocol = PROTOCOLS::OWNER_GRANT; 
                        ems.TriggerName = ""; 
                        ems.oblockName = request.requestorOblock; 
                        this->sendEM.write(ems); 
                    }
                    else{
                        DynamicMasterMessage dmm; 
                        dmm.protocol = PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE; 
                        dmm.info.controller = this->parentCont.at(dev); 
                        dmm.info.device = dev; 
                        dmm.info.oblock = oblockName; 
                        this->sendNM.write(dmm); 
                    }
                }
                this->triggerSet.clear(); 
                this->targetedDevices.clear();
            }

            break; 
        }
        case PROTOCOLS::OWNER_CONFIRM: {

            auto oblockName = DMM.info.oblock; 

            // If confirms this means the oblock is not waiting for state and the readerbox can close
            if(this->vTypesSchedule.contains(DMM.info.device)){
                //std::cout<<"Vtype confirm for: "<<DMM.info.device<<" for oblock "<<DMM.info.oblock<<std::endl; 
                auto& schedule = this->vTypesSchedule.at(DMM.info.device); 
                if(this->vTypesSchedule.at(DMM.info.device).owner == DMM.info.oblock){
                    EMStateMessage ems; 
                    HeapMasterMessage hmm;
                    hmm.protocol = PROTOCOLS::OWNER_CONFIRM_OK; 
                    hmm.info.device = DMM.info.device; 
                    hmm.info.oblock = oblockName;  
                    ems.dmm_list = {hmm}; 
                    ems.priority = -1; 
                    ems.protocol = PROTOCOLS::OWNER_CONFIRM_OK; 
                    ems.TriggerName = ""; 
                    ems.oblockName = DMM.info.oblock; 
                    this->sendEM.write(ems); 
                    schedule.queue.getQueue().pop(); 
                }
                else{
                    std::cout<<"Oblock stolen end confirmation"<<std::endl; 
                }
                break; 
            }
            
            //std::cout<<"Mailbox Owner Confirmation for the device: "<<DMM.info.device<<" from oblock "<<oblockName<<std::endl; 
            this->ConfContainer.send(MasterMailbox::buildDMM(DMM));
            break; 
        }
        case PROTOCOLS::OWNER_RELEASE:{
            auto oblockName = DMM.info.oblock; 
            this->oblockReadMap.at(DMM.info.oblock)->inExec = false; 

            if(this->vTypesSchedule.contains(DMM.info.device)){
                auto& scheduler = this->vTypesSchedule.at(DMM.info.device); 
                if(!scheduler.queue.getQueue().empty()){
                    // Send the next grant; 
                    auto& item = scheduler.queue.getQueue().top(); 
                    EMStateMessage ems; 
                    HeapMasterMessage hmm;
                    hmm.protocol = PROTOCOLS::OWNER_GRANT; 
                    hmm.info.device = DMM.info.device; 
                    hmm.info.oblock = item.requestorOblock;  
                    ems.dmm_list = {hmm}; 
                    ems.priority = -1; 
                    ems.protocol = PROTOCOLS::OWNER_GRANT; 
                    ems.TriggerName = ""; 
                    ems.oblockName = item.requestorOblock; 
                    //std::cout<<"Released vtype "<<DMM.info.oblock<<" with next item being "<<ems.oblockName<<std::endl; 
                    this->sendEM.write(ems); 
                }
                else{
                    //std::cout<<"Released vtype with Scheduler empty"<<std::endl; 
                    scheduler.isOwned = false; 
                }
                break; 
            }

            //std::cout<<"Mailbox Owner Release for the device: "<<DMM.info.device<<" from oblock "<<oblockName<<std::endl; 
            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
            break; 
        }
        case PROTOCOLS::PROCESS_EXEC :{
            auto oblockName = DMM.info.oblock;
            this->oblockReadMap[oblockName]->forwardPackets = false;
            this->oblockReadMap.at(oblockName)->inExec = true;
            break;
        }
        case PROTOCOLS::DISABLE_TRIGGER : {
            auto oblockName = DMM.info.oblock;
            //std::cout<<"Disabling trigger: "<<DMM.info.device<<" for oblock "<<DMM.info.oblock<<std::endl; 
            this->oblockReadMap.at(oblockName)->triggerMan.disableTrigger(DMM.info.device);
            break;
        }
        case PROTOCOLS::ENABLE_TRIGGER : {
            auto oblockName = DMM.info.oblock; 
            //std::cout<<"Disabling trigger: "<<DMM.info.device<<" for oblock "<<DMM.info.oblock<<std::endl; 
            this->oblockReadMap.at(oblockName)->triggerMan.enableTrigger(DMM.info.device);
            break; 
        }
        case PROTOCOLS::PULL_REQUEST : {
            DynamicMasterMessage pullDMM; 
            pullDMM.info = DMM.info; 
            pullDMM.protocol = PROTOCOLS::PULL_REQUEST;
            if(DMM.isCursor){
                this->ConfContainer.send(pullDMM, ""); 
            }
            else{
                this->ConfContainer.send(pullDMM); 
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
        //std::cout<<"Recieved from NM: "<<currentDMM.info.device<<std::endl; 
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
