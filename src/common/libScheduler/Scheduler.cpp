#include "Scheduler.hpp"
#include "include/Common.hpp"
#include <mutex>


/* 
    HELPER FUNCTIONS\
*/

HeapMasterMessage DeviceScheduler::makeMessage(TaskID& taskId, DeviceID& devName, PROTOCOLS pmsg, int priority = 0, bool vtype = false){
    HeapMasterMessage hmm; 
    hmm.info.device = devName; 
    hmm.info.task = taskId; 
    if(this->devControllerMap.contains(devName)){
         hmm.info.controller = this->devControllerMap[devName]; 
    }

    hmm.info.isVtype = vtype; 
    hmm.protocol = pmsg; 
    hmm.info.priority = priority; 

    return hmm; 
}


DeviceScheduler::DeviceScheduler(std::vector<TaskDescriptor> &taskDescList, std::function<void(HeapMasterMessage)> msgHandler)
{
    for(auto& taskDesc : taskDescList){
        auto& taskName = taskDesc.name;
        for(DeviceDescriptor& dev : taskDesc.outDevices){
            if(dev.deviceKind != DeviceKind::CURSOR){
                if(this->scheduledProcessMap.contains(dev.device_name)){
                    this->scheduledProcessMap[dev.device_name]; 
                }
                this->taskWaitMap[taskName].taskName = taskName;
                this->taskWaitMap[taskName].mustOwn.insert(dev.device_name); 
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
void DeviceScheduler::request(TaskID& requestor, int priority){
    // get the out devices from the task: 
    auto& jamar = this->taskWaitMap[requestor]; 
    auto& ownDevs = jamar.mustOwn;
    jamar.executeFlag = false; 
    
    if(ownDevs.empty()){ 
        std::string k = ""; 
        this->handleMessage(this->makeMessage(requestor, k, PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE)); 
        return; 
    }

    for(auto dev : ownDevs){
        this->handleMessage(this->makeMessage(requestor, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST, priority)); 
    }

    std::string k = ""; 
    this->handleMessage(this->makeMessage(requestor, k, PROTOCOLS::OWNER_CANDIDATE_REQUEST_CONCLUDE)); 

    // Wait until the final message arrives: 
    std::unique_lock<std::mutex> lock(jamar.mtx); 

    jamar.cv.wait(lock, [&jamar](){return jamar.executeFlag;});    
}

void DeviceScheduler::receive(HeapMasterMessage &recvMsg){
    switch(recvMsg.protocol){
        case PROTOCOLS::OWNER_GRANT :  {
            //std::cout<<"recieved grant for device: "<<recvMsg.info.device<<" for task: "<<recvMsg.info.task<<std::endl; 
            auto& targTask = recvMsg.info.task; 
            auto& targDevice = recvMsg.info.device; 
            auto& requestor = this->taskWaitMap.at(targTask);
            bool result = requestor.addDeviceGrant(targDevice); 
            //std::cout<<"Finished adding the grant"<<std::endl; 

            if(result){
               // std::cout<<"ready to send"<<std::endl; 
                auto& devList = this->taskWaitMap[targTask].mustOwn;
                for(auto dev : devList){
                   // std::cout<<"Sending confirm for device "<<dev<<" for task "<<targTask<<std::endl; 
                    HeapMasterMessage confirmMsg = makeMessage(targTask, dev, PROTOCOLS::OWNER_CONFIRM); 
                    this->handleMessage(confirmMsg); 
                } 
            }
            break; 
        }
        case PROTOCOLS::OWNER_CONFIRM_OK:{

            auto& targTask = recvMsg.info.task; 
            auto& targDevice = recvMsg.info.device; 
            bool result = this->taskWaitMap[targTask].confirmDevice(targDevice); 
            if(result){
                auto& devList = this->taskWaitMap[targTask].mustOwn;
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

void DeviceScheduler::release(TaskID& reqTask){
    //std::cout<<"Releasing devices for "<<reqTask<<std::endl; 
    auto& deviceList = this->taskWaitMap[reqTask].mustOwn;
    if(deviceList.empty()){
        std::string nullDev; 
        HeapMasterMessage newDMM = makeMessage(reqTask, nullDev, PROTOCOLS::OWNER_RELEASE_NULL); 
        this->handleMessage(newDMM); 
    }


    for(auto dev : deviceList){
        // Send each device that the task in question has ended its ownership
        HeapMasterMessage newDMM = makeMessage(reqTask, dev,PROTOCOLS::OWNER_RELEASE); 
        this->handleMessage(newDMM); 

        // Send the new item
        auto& scheDev = this->scheduledProcessMap[dev]; 
        if(!scheDev.QueueEmpty()){
            //std::cout<<"Sending new state"<<std::endl; 
            auto newReq = scheDev.QueuePop(); 
            HeapMasterMessage hmm = makeMessage(newReq.requestorTask, dev, PROTOCOLS::OWNER_CANDIDATE_REQUEST); 
            this->handleMessage(hmm); 
        }
    }
    
}