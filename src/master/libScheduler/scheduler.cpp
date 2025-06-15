#include "scheduler.hpp"




/* 
    HELPER FUNCTIONS\
*/

DynamicMasterMessage DeviceScheduler::makeMessage(OblockID& oName, DeviceID& devName, PROTOCOLS pmsg){
    DynamicMasterMessage dmm; 
    dmm.info.device = devName; 
    dmm.info.oblock = oName; 
    dmm.info.controller = this->devControllerMap[devName]; 
    dmm.info.isVtype = false; // Idk this doesn't matter
    dmm.protocol = pmsg; 

    return dmm; 
}


DeviceScheduler::DeviceScheduler(std::vector<OBlockDesc> &oblockDescList, TSQ<DynamicMasterMessage> &inSendMM)
: sendMM(inSendMM) 
{
    for(auto& odesc : oblockDescList){
        auto& oblockName = odesc.name;
        for(DeviceDescriptor& dev : odesc.outDevices){
            if(!dev.isCursor){
                this->oblockWaitMap[oblockName].mustOwn.insert(dev.device_name); 
                this->devControllerMap[dev.device_name] = dev.controller; 
            }
        }
    }
}

// void updates the scheduler

/*
    May need to add a now owns 
*/

// Sends a request message for each device stae
void DeviceScheduler::request(SchedulerReq& reqOblock){
    // get the out devices from the oblock: 
    auto& jamar = this->oblockWaitMap[reqOblock.requestorOblock]; 
    auto& ownDevs = jamar.mustOwn;
    for(auto dev : ownDevs){
        auto& scheData = this->scheduledProcessMap[dev]; 
        if(scheData.loadedRequest.ps == PROCSTATE::EXECUTED){
            reqOblock.ps = PROCSTATE::LOADED; 
            scheData.loadedRequest = reqOblock; 
            this->sendMM.write(this->makeMessage(reqOblock.requestorOblock, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST)); 
        }
        else if(reqOblock.priority > scheData.loadedRequest.priority){
            // Candidate replacement
            scheData.pendingRequests.push(scheData.loadedRequest);
            scheData.loadedRequest = reqOblock;  
            this->sendMM.write(this->makeMessage(reqOblock.requestorOblock, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST)); 
        }
        else{
            scheData.pendingRequests.push(reqOblock); 
        }
    }

    // Wait until the final message arrives: 
    std::unique_lock<std::mutex> lock(jamar.mtx); 
    jamar.cv.wait(lock, [&jamar](){return jamar.executeFlag;});   
}

void DeviceScheduler::receive(DynamicMasterMessage &recvMsg){
    switch(recvMsg.protocol){
        case PROTOCOLS::OWNER_GRANT :  {
            auto& targOblock = recvMsg.info.oblock; 
            auto& targDevice = recvMsg.info.device; 
            bool result = this->oblockWaitMap[targOblock].addDevice(targDevice); 
            if(result){
              auto& devList = this->oblockWaitMap[targOblock].mustOwn;

              for(auto dev : devList){
                DynamicMasterMessage confirmMsg = makeMessage(targOblock, dev, PROTOCOLS::OWNER_CONFIRM); 
                this->sendMM.write(confirmMsg); 
                auto& devProc = this->scheduledProcessMap[dev];  
                
                // Perform any time based schdueling algorithms etc

                if(!devProc.pendingRequests.empty()){
                    auto nextReq = devProc.pendingRequests.top(); 
                    devProc.pendingRequests.pop(); 
                    nextReq.ps = PROCSTATE::LOADED; 
                    devProc.loadedRequest = nextReq;  
                }
                else{
                    devProc.loadedRequest.ps = PROCSTATE::EXECUTED; 
                }
              } 
            }
            break; 
        }
        default : {
            std::cout<<"Unknown protocol message directed at scheduler"<<std::endl; 
            break; 
        }
    }
}

void DeviceScheduler::release(OblockID& reqOblock){
    auto& deviceList = this->oblockWaitMap[reqOblock].mustOwn;
    for(auto dev : deviceList){
        // Send each device that the oblock in question has ended its ownership
        DynamicMasterMessage newDMM = makeMessage(reqOblock, dev, PROTOCOLS::OWNER_RELEASE); 
    }
}