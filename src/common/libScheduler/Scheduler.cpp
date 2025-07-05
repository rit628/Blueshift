#include "Scheduler.hpp"
#include "include/Common.hpp"


/* 
    HELPER FUNCTIONS\
*/

HeapMasterMessage DeviceScheduler::makeMessage(OblockID& oName, DeviceID& devName, PROTOCOLS pmsg){
    HeapMasterMessage hmm; 
    hmm.info.device = devName; 
    hmm.info.oblock = oName; 
    hmm.info.controller = this->devControllerMap[devName]; 
    hmm.info.isVtype = false; // Idk this doesn't matter
    hmm.protocol = pmsg; 

    return hmm; 
}


DeviceScheduler::DeviceScheduler(std::vector<OBlockDesc> &oblockDescList, std::function<void(HeapMasterMessage)> msgHandler)
{
    for(auto& odesc : oblockDescList){
        auto& oblockName = odesc.name;
        for(DeviceDescriptor& dev : odesc.outDevices){
            if(!dev.isCursor){
                if(this->scheduledProcessMap.contains(dev.device_name)){
                    this->scheduledProcessMap[dev.device_name]; 
                }
                this->oblockWaitMap[oblockName].mustOwn.insert(dev.device_name); 
                this->devControllerMap[dev.device_name] = dev.controller; 
            }
        }
    }

    this->handleMessage = msgHandler; 
}
/*
    May need to add a now owns 
*/
// Sends a request message for each device stae
void DeviceScheduler::request(OblockID& requestor, int priority){
    // get the out devices from the oblock: 
    auto& jamar = this->oblockWaitMap[requestor]; 
    auto& ownDevs = jamar.mustOwn;
    jamar.executeFlag = false; 
    std::cout<<"Request made from oblock: "<<requestor<<std::endl; 

    if(ownDevs.empty()){
        std::cout<<"No devices left to own"<<std::endl; 
        return; 
    }

    for(auto dev : ownDevs){
        auto& scheData = this->scheduledProcessMap[dev]; 

        SchedulerReq loadRequest; 
        loadRequest.requestorOblock = requestor; 
        loadRequest.priority = priority; 
        loadRequest.ctl = "master"; 
        loadRequest.targetDevice = dev; 
 
        if(scheData.loadedRequest.ps == PROCSTATE::EXECUTED){
            loadRequest.ps = PROCSTATE::LOADED; 
            scheData.loadedRequest = loadRequest; 
            this->handleMessage(this->makeMessage(loadRequest.requestorOblock, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST)); 
        }
        else if(priority > scheData.loadedRequest.priority){
            // Candidate replacement
            scheData.QueuePush(scheData.loadedRequest);
            loadRequest.ps = PROCSTATE::LOADED; 
            scheData.loadedRequest = loadRequest;  
            this->handleMessage(this->makeMessage(loadRequest.requestorOblock, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST)); 
        }
        else{
            loadRequest.ps = PROCSTATE::WAITING; 
            scheData.QueuePush(loadRequest); 
        }
    }

    // Wait until the final message arrives: 
    std::unique_lock<std::mutex> lock(jamar.mtx); 
    jamar.cv.wait(lock, [&jamar](){return jamar.executeFlag;});   
}

void DeviceScheduler::receive(HeapMasterMessage &recvMsg){
    switch(recvMsg.protocol){
        case PROTOCOLS::OWNER_GRANT :  {
            auto& targOblock = recvMsg.info.oblock; 
            auto& targDevice = recvMsg.info.device; 
            std::cout<<"SCHEDULER Received write for device: "<<recvMsg.info.device<<std::endl; 
            bool result = this->oblockWaitMap[targOblock].addDevice(targDevice); 
            if(result){std::cout<<"Run for "<<targOblock<<std::endl;} 
            if(result){
              auto& devList = this->oblockWaitMap[targOblock].mustOwn;
              for(auto dev : devList){
                std::cout<<"Oblock: "<<recvMsg.info.oblock<<" calling for: "<<dev<<std::endl; 
                HeapMasterMessage confirmMsg = makeMessage(targOblock, dev, PROTOCOLS::OWNER_CONFIRM); 
                this->handleMessage(confirmMsg); 
              } 
            }
            break; 
        }
        case PROTOCOLS::OWNER_CONFIRM_OK:{

            auto& targOblock = recvMsg.info.oblock; 
            auto& targDevice = recvMsg.info.device; 
            bool result = this->oblockWaitMap[targOblock].confirmDevice(targDevice); 
            if(result){
                auto& devList = this->oblockWaitMap[targOblock].mustOwn;
                for(auto dev : devList){
                    auto& devProc = this->scheduledProcessMap[dev];  
                    
                    // Perform any time based schdueling algorithms etc

                    if(!devProc.QueueEmpty()){
                        auto nextReq = devProc.QueuePop(); 
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
    std::cout<<"Releasing devices for "<<reqOblock<<std::endl; 
    auto& deviceList = this->oblockWaitMap[reqOblock].mustOwn;
    for(auto dev : deviceList){
        // Send each device that the oblock in question has ended its ownership
        HeapMasterMessage newDMM = makeMessage(reqOblock, dev,PROTOCOLS::OWNER_RELEASE); 
        this->handleMessage(newDMM); 
    }
}