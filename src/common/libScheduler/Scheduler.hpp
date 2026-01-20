#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <unordered_map> 
#include <unordered_set> 
#include <vector>
#include <algorithm>
#include <set> 


/*
    Waiting: The task is in the queue for all devices
    Running: The task is in exeuction
    Loaded: The task has been granted access to at least one device and is waiting on another
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


    private: 

        void triggerConfirm(){
            {
                std::lock_guard<std::mutex> lock(mtx); 
                executeFlag = true;
            }
            cv.notify_one(); 
        }

        
    public: 

    std::unordered_set<DeviceID> mustOwn; 

    std::unordered_set<DeviceID> grantedDevices; 
    std::unordered_set<DeviceID> ownedDevices; 
    int ownedCounter = 0; 
    int confirmedCounter = 0; 
    bool executeFlag = false; 
    TaskID taskName; 


    std::mutex mtx; 
    std::condition_variable cv;

    // Receiver functions(react to message)

    bool addDeviceGrant(DeviceID &dev){
        if(mustOwn.contains(dev)){
            grantedDevices.insert(dev); 
           if(mustOwn.size() == ++this->ownedCounter){
            ownedCounter = 0;  
            //std::cout<<"CONFIRMED OWNER"<<std::endl; 
            return true;    
           } 
        }
        return false; 
    }

    bool confirmDevice(DeviceID &dev){
        if(mustOwn.contains(dev)){
            this->ownedDevices.insert(dev);
            if(mustOwn.size() == ++this->confirmedCounter){
                this->ownedDevices.clear(); 
                this->confirmedCounter = 0; 
                //std::cout<<"we good! running task"<<std::endl; 
                this->triggerConfirm();
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

        // Maps the Task to the list of devices that it writes to 
        std::unordered_map<DeviceID, DeviceQueue> scheduledProcessMap; 

        // Returns what each task is waiting on
        std::unordered_map<TaskID, PendingStateInfo> taskWaitMap; 

        // Utility Functions: 
        HeapMasterMessage makeMessage(TaskID& oName, DeviceID& devName, PROTOCOLS pmsg, int priority, bool vtype); 

        // Function
        std::function<void(HeapMasterMessage)> handleMessage; 
        

    public: 
        DeviceScheduler(std::vector<TaskDescriptor> &taskDescList, std::function<void(HeapMasterMessage)> dmm_message); 
        void request(TaskID& taskName, int priority); 
        void receive(HeapMasterMessage &DMM); 
        void release(TaskID &reqTask); 
}; 


