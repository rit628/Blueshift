#include "Scheduler.hpp"
#include "include/Common.hpp"
#include <mutex>


/* 
    HELPER FUNCTIONS\
*/

HeapMasterMessage DeviceScheduler::makeMessage(OblockID& oName, DeviceID& devName, PROTOCOLS pmsg, int priority = 0){
    HeapMasterMessage hmm; 
    hmm.info.device = devName; 
    hmm.info.oblock = oName; 
    if(this->devControllerMap.contains(devName)){
         hmm.info.controller = this->devControllerMap[devName]; 
    }
    hmm.info.isVtype = false; // Idk this doesn't matter
    hmm.protocol = pmsg; 
    hmm.info.priority = priority; 

    return hmm; 
}


DeviceScheduler::DeviceScheduler(std::vector<OBlockDesc> &oblockDescList, std::function<void(HeapMasterMessage)> msgHandler)
{
    for(auto& odesc : oblockDescList){
        auto& oblockName = odesc.name;
        for(DeviceDescriptor& dev : odesc.outDevices){
            if(!dev.isCursor && !dev.isVtype){
                if(this->scheduledProcessMap.contains(dev.device_name)){
                    this->scheduledProcessMap[dev.device_name]; 
                }
                this->oblockWaitMap[oblockName].oblockName = oblockName;
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
    std::cout<<"priority: "<<std::endl;
    // get the out devices from the oblock: 
    auto& jamar = this->oblockWaitMap[requestor]; 
    auto& ownDevs = jamar.mustOwn;
    jamar.executeFlag = false; 
    std::cout<<"Request made from oblock: "<<requestor<<std::endl; 

    if(ownDevs.empty()){
        std::cout<<"No devices left to own"<<std::endl; 
        std::string k = ""; 
        this->handleMessage(this->makeMessage(requestor, k, PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE)); 
        return; 
    }

    for(auto dev : ownDevs){
        this->handleMessage(this->makeMessage(requestor, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST, priority)); 
    }

    string k = ""; 
    this->handleMessage(this->makeMessage(requestor, k, PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE)); 

    // Wait until the final message arrives: 
    std::unique_lock<std::mutex> lock(jamar.mtx); 

    jamar.cv.wait(lock, [&jamar](){return jamar.executeFlag;});    
}

void DeviceScheduler::receive(HeapMasterMessage &recvMsg){
    switch(recvMsg.protocol){
        case PROTOCOLS::OWNER_GRANT :  {
            auto& targOblock = recvMsg.info.oblock; 
            auto& targDevice = recvMsg.info.device; 
            auto& requestor = this->oblockWaitMap.at(targOblock);
            bool result = requestor.addDeviceGrant(targDevice); 
            if(result){
                auto& devList = this->oblockWaitMap[targOblock].mustOwn;
                for(auto dev : devList){
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

        // Send the new item
        auto& scheDev = this->scheduledProcessMap[dev]; 
        if(!scheDev.QueueEmpty()){
            std::cout<<"Sending new state"<<std::endl; 
            auto newReq = scheDev.QueuePop(); 
            HeapMasterMessage hmm = makeMessage(newReq.requestorOblock, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST); 
            this->handleMessage(hmm); 
        }
    }
    
}