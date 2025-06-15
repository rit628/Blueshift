#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include <mutex>
#include <queue>
#include <unordered_map> 
#include <vector>


using ControllerID = std::string; 
using DeviceID = std::string; 
using OblockID = std::string; 

/*
    Waiting: The oblock is in the queue for all devices
    Running: The oblock is in exeuction
    Loaded: The oblock has been granted access to at least one device and is waiting on another
*/


// ScheduleRequestMetadata
enum class PROCSTATE{
    WAITING, 
    EXECUTED, 
    LOADED, 
}; 


// Scheduler Request State
struct SchedulerReq{
    OblockID requestorOblock; 
    DeviceID targetDevice; 
    int priority; 
    PROCSTATE ps;     
    int cyclesWaiting; 
}; 

// Req Scheduler used by 
struct ReqComparator{
    bool operator()(const SchedulerReq& a, const SchedulerReq& b) const {
        return a.priority > b.priority; 
    }
}; 

struct DeviceQueue{
    // Currently Loaded request from the Compiler
    SchedulerReq loadedRequest; 
    std::priority_queue<SchedulerReq, std::vector<SchedulerReq>, ReqComparator> pendingRequests; 
}; 


// Loaded state info: 
struct PendingStateInfo{
    std::unordered_set<DeviceID> mustOwn; 
    int ownedCounter = 0; 
    bool executeFlag = false; 

    std::mutex mtx; 
    std::condition_variable cv; 

    bool addDevice(DeviceID &dev){
        if(mustOwn.contains(dev)){
           if(mustOwn.size() == ++this->ownedCounter){
            ownedCounter = 0;  
            {
                std::lock_guard<std::mutex> lock(mtx); 
                executeFlag = true;
            }
            cv.notify_one(); 
            return true;    
           } 
        }
        return false; 
    }
}; 

class DeviceScheduler{
    private: 
    
        // Maps deviceID to CTL
        std::unordered_map<DeviceID, ControllerID> devControllerMap; 

        // Maps the Oblock to the list of devices that it writes to 
        std::unordered_map<DeviceID, DeviceQueue> scheduledProcessMap; 

        // Returns what each oblock is waiting on
        std::unordered_map<OblockID, PendingStateInfo> oblockWaitMap; 

        // sendMM (for sending messages)
        TSQ<DynamicMasterMessage> &sendMM; 

        // Utility Functions: 
        DynamicMasterMessage makeMessage(OblockID& oName, DeviceID& devName, PROTOCOLS pmsg); 
        

    public: 
        DeviceScheduler(std::vector<OBlockDesc> &oblockDescList, TSQ<DynamicMasterMessage> &sendMM); 
        void request(SchedulerReq &reqOblock); 
        void receive(DynamicMasterMessage &DMM); 
        void release(OblockID &reqOblock); 
}; 


