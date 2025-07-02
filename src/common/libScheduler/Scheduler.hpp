#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <unordered_map> 
#include <unordered_set> 
#include <vector>
#include <set> 


/*
    Waiting: The oblock is in the queue for all devices
    Running: The oblock is in exeuction
    Loaded: The oblock has been granted access to at least one device and is waiting on another
*/


struct DeviceQueue{
    // Currently Loaded request from the master
    private: 
        std::priority_queue<SchedulerReq, std::vector<SchedulerReq>, ReqComparator> pendingRequests;
        // Lock to maintain thread saftey on pendingRequest priority queue 
        mutable std::shared_mutex mut;
    
    public: 
        SchedulerReq loadedRequest; 

        void QueuePush(SchedulerReq &newReq){
            std::shared_lock<std::shared_mutex> lock(mut); 
            pendingRequests.push(newReq); 
        }

        SchedulerReq QueuePop(){
            std::shared_lock<std::shared_mutex> lock(mut); 
            SchedulerReq topVal = pendingRequests.top(); 
            pendingRequests.pop(); 
            return topVal; 

        }

        bool QueueEmpty(){
            std::shared_lock<std::shared_mutex> lock(mut); 
            return pendingRequests.empty(); 
        }
}; 



// Loaded state info: 
struct PendingStateInfo{
    std::unordered_set<DeviceID> mustOwn; 
    int ownedCounter = 0; 
    int confirmedCounter = 0; 
    bool executeFlag = false; 


    std::mutex mtx; 
    std::condition_variable cv; 

    bool addDevice(DeviceID &dev){
        if(mustOwn.contains(dev)){
           if(mustOwn.size() == ++this->ownedCounter){
            ownedCounter = 0;  
            return true;    
           } 
        }
        return false; 
    }

    // Used when device confirmation is received
    bool confirmDevice(DeviceID &dev){
        if(mustOwn.contains(dev)){
            if(mustOwn.size() == ++this->confirmedCounter){
                this->confirmedCounter = 0; 
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

        // Utility Functions: 
        HeapMasterMessage makeMessage(OblockID& oName, DeviceID& devName, PROTOCOLS pmsg); 

        // Function
        function<void(HeapMasterMessage)> handleMessage; 
        

    public: 
        DeviceScheduler(std::vector<OBlockDesc> &oblockDescList, function<void(HeapMasterMessage)> dmm_message); 
        void request(OblockID& oblockName, int priority); 
        void receive(HeapMasterMessage &DMM); 
        void release(OblockID &reqOblock); 
}; 


