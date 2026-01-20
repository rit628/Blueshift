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


MasterMailbox::MasterMailbox(std::vector<TaskDescriptor> TaskList, TSQ<DynamicMasterMessage> &readNM, 
    TSQ<HeapMasterMessage> &readEM, TSQ<DynamicMasterMessage> &sendNM, TSQ<EMStateMessage> &sendEM)
: readNM(readNM), readEM(readEM), sendEM(sendEM), sendNM(sendNM),  ConfContainer(sendNM ,TaskList)
{
    this->TaskList = TaskList;
    std::unordered_set<std::string> emplaced_set; 
    

    // Creating the read line
    for(auto &task : this->TaskList)
    {
        taskReadMap[task.name] = make_unique<ReaderBox>(task.name, task, this->triggerSet, sendEM);

        for(auto &devDesc : task.binded_devices)
        { 
            std::string deviceName = devDesc.device_name;
            auto TSQPtr = std::make_shared<TSQ<HeapMasterMessage>>();
            auto& db =taskReadMap[task.name]->waitingQs[deviceName];
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
                taskReadMap[task.name]->insertState(hmm); 
            } 
        }

        // Write the out devics
        for(auto& devDesc : task.binded_devices){
            DeviceID devName = devDesc.device_name;
            if(devDesc.deviceKind == DeviceKind::CURSOR){
                devName = devName + "::" + task.name; 
            } 
            if(!emplaced_set.contains(devName)){
                auto wb = std::make_unique<WriterBox>(devDesc,  sendNM, this->ConfContainer);
                wb->deviceName = devName; 
                wb->waitingForCallback = false; 
                this->parentCont[devName] = devDesc.controller; 
                this->deviceWriteMap[devName] = std::move(wb); 
                emplaced_set.insert(devName); 
            }
        }
    }


    // populate the device to task_set mapping for interrupt devices (as state not copied by Timer system)
    for(auto &task : TaskList)
    {
        for(auto &devDesc : task.binded_devices){
            std::string deviceName = devDesc.device_name;
            interruptName_map[deviceName].push_back(task.name);   
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

ReaderBox::ReaderBox(std::string name, TaskDescriptor& taskDesc, std::unordered_set<TaskID> &triggerSet, TSQ<EMStateMessage> &ems)
: triggerSet(triggerSet), triggerMan(taskDesc), sendEM(ems)
{
    this->TaskName = name;
    this->taskDesc = taskDesc; 

    for(auto& desc : taskDesc.binded_devices){
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
            for(auto &taskName : this->interruptName_map[devName]){
                if(this->taskReadMap.at(taskName)->waitingQs.contains(devName)){  
                    DMM.info.task = taskName; 
                    HeapMasterMessage hmm; 
                    hmm.protocol = PROTOCOLS::CALLBACKRECIEVED; 
                    hmm.info = DMM.info; 
                    hmm.heapTree = DMM.DM.toTree(); 
                    this->taskReadMap[taskName]->insertState(hmm); 
                }
            }
            for(auto& taskName : this->interruptName_map[devName]){
                this->taskReadMap[taskName]->handleRequest(); 
            } 
        }
        else{
            auto& readMap = this->taskReadMap.at(DMM.info.task); 
            readMap->insertState(DMM); 
            readMap->handleRequest(); 
        }  

        if(DMM.isCursor){
            devName = devName + "::" + DMM.info.task; 
        }
        deviceWriteMap.at(devName)->notifyCallBack(); 
        
            break;
        }
        case PROTOCOLS::SENDSTATES:
        {
            if(DMM.isInterrupt){
                std::vector<TaskID> taskList = this->interruptName_map[DMM.info.device];
                for(auto& taskId : taskList){
                    if(!this->taskReadMap.contains(taskId)){break;}
                    auto& targReadBox = this->taskReadMap[taskId]; 
                    DMM.info.task = taskId; 

                    HeapMasterMessage newHMM(DMM); 
                    targReadBox->insertState(newHMM); 
                } 
                //std::cout<<"Tasks Triggered: "<<this->triggerSet.size()<<std::endl; 

                for(auto& taskId : taskList){
                    if(!this->taskReadMap.contains(taskId)){break;}
                    auto& targReadBox = this->taskReadMap[taskId]; 
                    targReadBox->handleRequest(); 
                }
            }
            else{
                TaskID targId = DMM.info.task; 
                if(!this->taskReadMap.contains(targId)){break;}
                auto& rBox = this->taskReadMap.at(targId); 
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
            ems.taskName = DMM.info.task; 
            this->sendEM.write(ems); 
            break; 
        }
        case PROTOCOLS::PULL_RESPONSE:{
            EMStateMessage ems;
            ems.dmm_list = {DMM};
            ems.priority = -1; 
            ems.protocol = DMM.protocol;
            ems.TriggerName = ""; 
            ems.taskName = DMM.info.task;
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
    ReaderBox &correspondingReaderBox = *taskReadMap.at(DMM.info.task);
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
                if(this->vTypesSchedule.at(DMM.info.device).owner != DMM.info.task){
                    std::cout<<"Write attempted by unowned task"<<std::endl; 
                    break; 
                }

                std::vector<TaskID> taskList = this->interruptName_map.at(DMM.info.device); 
                for(auto &name : taskList){
                    if(!this->taskReadMap.contains(name)){break;}
                    DMM.protocol = PROTOCOLS::CALLBACKRECIEVED; 
                    std::unique_ptr<ReaderBox>& reader = this->taskReadMap[name]; 
                    reader->insertState(DMM); 
                }   
           
                for(auto& name : taskList){
                    if(!this->taskReadMap.contains(name)){break;}
                    std::unique_ptr<ReaderBox>& reader = this->taskReadMap[name]; 
                    reader->handleRequest();
                }
              
                break; 
            }

            if(DMM.isCursor){
                dev = dev + "::" + DMM.info.task; 
            }

            WriterBox &assignedBox = *deviceWriteMap.at(dev);
            auto owPolicy = taskReadMap.at(DMM.info.task)->waitingQs.at(DMM.info.device).overwritePolicy;

            assignedBox.writeOut(DMM, owPolicy, PROTOCOLS::SENDSTATES); 
            break;
        }
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST:{   
            auto taskName = DMM.info.task; 
            //std::cout<<"Mailbox Ownership request for the device: "<<DMM.info.device<<" from task "<<taskName<<std::endl; 
            this->taskReadMap[taskName]->forwardPackets = true; 
            this->targetedDevices.insert(DMM.info.device); 

            if(this->vTypesSchedule.contains(DMM.info.device)){
                //std::cout<<"Vtype candidate request for: "<<DMM.info.device<<" from task "<<DMM.info.task<<std::endl; 
                SchedulerReq req; 
                req.requestorTask = DMM.info.task;
                req.targetDevice = DMM.info.device; 
                this->vTypesSchedule.at(DMM.info.device).queue.getQueue().push(req); 
                break; 
            }

            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
            break; 
        }
        case PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE:{
        
            auto taskName = DMM.info.task; 
            if(this->triggerSet.contains(taskName)){
                this->triggerSet.erase(taskName); 
            }

            if(triggerSet.empty()){
                for(auto& dev : targetedDevices){
                    if(this->vTypesSchedule.contains(dev)){
                        auto& schedule = this->vTypesSchedule.at(dev);
                        auto request = schedule.queue.getQueue().top();
                        schedule.owner = request.requestorTask; 
                        schedule.isOwned = false; 
                        TaskID targetTask = request.requestorTask;
                        //std::cout<<"Sending vtype grant for task "<<targetTask<<" for vtype "<<dev<<std::endl;
                        EMStateMessage ems; 
                        HeapMasterMessage hmm;
                        hmm.protocol = PROTOCOLS::OWNER_GRANT; 
                        hmm.info.device = dev; 
                        hmm.info.task = targetTask;  
                        ems.dmm_list = {hmm}; 
                        ems.priority = -1; 
                        ems.protocol = PROTOCOLS::OWNER_GRANT; 
                        ems.TriggerName = ""; 
                        ems.taskName = request.requestorTask; 
                        this->sendEM.write(ems); 
                    }
                    else{
                        DynamicMasterMessage dmm; 
                        dmm.protocol = PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE; 
                        dmm.info.controller = this->parentCont.at(dev); 
                        dmm.info.device = dev; 
                        dmm.info.task = taskName; 
                        this->sendNM.write(dmm); 
                    }
                }
                this->triggerSet.clear(); 
                this->targetedDevices.clear();
            }

            break; 
        }
        case PROTOCOLS::OWNER_CONFIRM: {

            auto taskName = DMM.info.task; 

            // If confirms this means the task is not waiting for state and the readerbox can close
            if(this->vTypesSchedule.contains(DMM.info.device)){
                //std::cout<<"Vtype confirm for: "<<DMM.info.device<<" for task "<<DMM.info.task<<std::endl; 
                auto& schedule = this->vTypesSchedule.at(DMM.info.device); 
                if(this->vTypesSchedule.at(DMM.info.device).owner == DMM.info.task){
                    EMStateMessage ems; 
                    HeapMasterMessage hmm;
                    hmm.protocol = PROTOCOLS::OWNER_CONFIRM_OK; 
                    hmm.info.device = DMM.info.device; 
                    hmm.info.task = taskName;  
                    ems.dmm_list = {hmm}; 
                    ems.priority = -1; 
                    ems.protocol = PROTOCOLS::OWNER_CONFIRM_OK; 
                    ems.TriggerName = ""; 
                    ems.taskName = DMM.info.task; 
                    this->sendEM.write(ems); 
                    schedule.queue.getQueue().pop(); 
                }
                else{
                    std::cout<<"Task stolen end confirmation"<<std::endl; 
                }
                break; 
            }
            
            //std::cout<<"Mailbox Owner Confirmation for the device: "<<DMM.info.device<<" from task "<<taskName<<std::endl; 
            this->ConfContainer.send(MasterMailbox::buildDMM(DMM));
            break; 
        }
        case PROTOCOLS::OWNER_RELEASE_NULL:{
            auto taskName = DMM.info.task; 
            this->taskReadMap.at(DMM.info.task)->inExec = false; 
            break; 
        }
        case PROTOCOLS::OWNER_RELEASE:{
            auto taskName = DMM.info.task; 
            this->taskReadMap.at(DMM.info.task)->inExec = false; 

            if(this->vTypesSchedule.contains(DMM.info.device)){
                auto& scheduler = this->vTypesSchedule.at(DMM.info.device); 
                if(!scheduler.queue.getQueue().empty()){
                    // Send the next grant; 
                    auto& item = scheduler.queue.getQueue().top(); 
                    EMStateMessage ems; 
                    HeapMasterMessage hmm;
                    hmm.protocol = PROTOCOLS::OWNER_GRANT; 
                    hmm.info.device = DMM.info.device; 
                    hmm.info.task = item.requestorTask;  
                    ems.dmm_list = {hmm}; 
                    ems.priority = -1; 
                    ems.protocol = PROTOCOLS::OWNER_GRANT; 
                    ems.TriggerName = ""; 
                    ems.taskName = item.requestorTask; 
                    //std::cout<<"Released vtype "<<DMM.info.task<<" with next item being "<<ems.taskName<<std::endl; 
                    this->sendEM.write(ems); 
                }
                else{
                    //std::cout<<"Released vtype with Scheduler empty"<<std::endl; 
                    scheduler.isOwned = false; 
                }
                break; 
            }

            //std::cout<<"Mailbox Owner Release for the device: "<<DMM.info.device<<" from task "<<taskName<<std::endl; 
            this->sendNM.write(MasterMailbox::buildDMM(DMM)); 
            break; 
        }
        case PROTOCOLS::PROCESS_EXEC :{
            auto taskName = DMM.info.task;
            this->taskReadMap[taskName]->forwardPackets = false;
            this->taskReadMap.at(taskName)->inExec = true;
            break;
        }
        case PROTOCOLS::DISABLE_TRIGGER : {
            auto taskName = DMM.info.task;
            //std::cout<<"Disabling trigger: "<<DMM.info.device<<" for task "<<DMM.info.task<<std::endl; 
            this->taskReadMap.at(taskName)->triggerMan.disableTrigger(DMM.info.device);
            break;
        }
        case PROTOCOLS::ENABLE_TRIGGER : {
            auto taskName = DMM.info.task; 
            //std::cout<<"Disabling trigger: "<<DMM.info.device<<" for task "<<DMM.info.task<<std::endl; 
            this->taskReadMap.at(taskName)->triggerMan.enableTrigger(DMM.info.device);
            break; 
        }
        case PROTOCOLS::PULL_REQUEST : {
            DynamicMasterMessage pullDMM; 
            pullDMM.info = DMM.info; 
            pullDMM.isCursor = pullDMM.isCursor; 
            pullDMM.protocol = PROTOCOLS::PULL_REQUEST;
            if(DMM.isCursor){
                this->ConfContainer.send(pullDMM, DMM.info.task); 
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
