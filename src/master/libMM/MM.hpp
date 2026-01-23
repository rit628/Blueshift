#pragma once

#include "Serialization.hpp"
#include "TSQ.hpp"
#include <bitset>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>



using TaskID = std::string;
using DeviceID = std::string; 

# define MAX_EM_QUEUE_FILL 10 
#define BITSET_SZ 32
 
/*
    Contains a single dmsg (without needing the )
*/
struct AtomicDMMContainer{
    private: 
        HeapMasterMessage dmsg; 
        bool hasItem = false; 
        std::mutex mux; 
    
    public: 
        void replace(HeapMasterMessage new_dmsg){
            std::scoped_lock<std::mutex> lock(mux); 
            hasItem = true; 
            this->dmsg = new_dmsg; 
        }

        HeapMasterMessage get(){
            std::scoped_lock<std::mutex> lock(mux); 
            return this->dmsg; 
        }

        bool isInit(){
            return this->hasItem; 
        }
}; 

class TriggerManager{
    private:   
        std::bitset<BITSET_SZ> currentBitmap;  
        std::bitset<BITSET_SZ> initBitmap; 
        std::unordered_map<std::string, int> stringMap; 
        std::vector<std::bitset<BITSET_SZ>> ruleset; 
        std::vector<TriggerData> trigData; 
        std::bitset<BITSET_SZ> defaultBitstring; 
        std::unordered_set<int> excludedTriggers; 
        // Maps trigger indices to integers
        std::unordered_map<std::string, int> triggerIndexMap; 

    
    public: 
        // Device Constructor (created rules)
        TriggerManager(TaskDescriptor& TaskDesc){
            int i = 0; 
            for(DeviceDescriptor& devDesc : TaskDesc.binded_devices){
                stringMap[devDesc.device_name] = i; 
                i++; 
            }

            // Add the default universal bitset (all 1s): 
            defaultBitstring = (1 << (i)) - 1; 
            this->trigData = TaskDesc.triggers; 
            i = 0;
            // Loop through rules; 
            for(auto& data : TaskDesc.triggers)
            {
                auto& rule = data.rule;
                std::bitset<BITSET_SZ> king; 
                for(auto& devName : rule){
                    king.set(this->stringMap[devName]); 
                }       
                this->triggerIndexMap[data.id] = i; 
                ruleset.push_back(king); 
                i++; 
            }
        }

        private: 
            // Tests bit against ruleset and grab the trigger rule with highest priority: 
            bool testBit(int& id){
                int i = 0; 
                int max_priority = -1; 
                for(auto& ruleBit : this->ruleset){
                    if(excludedTriggers.contains(i)){
                        continue;
                    }

                    if((this->currentBitmap & ruleBit) == ruleBit){
                        if(this->trigData[i].priority > max_priority){
                            max_priority = this->trigData[i].priority;
                            id = i; 
                        }
                    }
                    i++; 
                }

                if(max_priority > 0 || (this->currentBitmap & this->defaultBitstring) == this->defaultBitstring){
                    return true; 
                }

                // Check if the reultBit is the default (all devices)
                return false;  
            }
        public: 
            // Returns true if the new device corresponds to a trigger 
            bool processDevice(std::string &object, int& trigger_id){
                int filledMap = pow(2, stringMap.size()) - 1; 
                if(this->initBitmap.to_ulong() != filledMap){
                    this->initBitmap.set(this->stringMap[object]);
                    // force a trigger when the map is init bitmap is filled
                    if(this->initBitmap.to_ulong() == filledMap){
                        // Code for initial trigger
                        trigger_id = -1; 
                        return true; 
                    } 
                    return false; 
                }
                
                this->currentBitmap.set(this->stringMap[object]);         
                bool found = this->testBit(trigger_id); 
                if(found){
                    this->currentBitmap.reset(); 
    
                    return true; 
                }
                return false; 
            }

            void debugPrintRules(){
                for(auto& rule : this->ruleset){
                    std::cout<<rule<<std::endl; 
                }
            }

            void debugCurrentString(){
                std::cout<<this->currentBitmap<<std::endl; 
            }

            void disableTrigger(std::string &triggerName){
                int index = this->triggerIndexMap.at(triggerName); 
                this->excludedTriggers.insert(index); 
            }

            void enableTrigger(std::string &triggerName){
                int index = this->triggerIndexMap.at(triggerName);
                if(this->excludedTriggers.contains(index)){
                    this->excludedTriggers.erase(index);
                }
            }
}; 


struct DeviceBox{
    std::shared_ptr<TSQ<HeapMasterMessage>> stateQueues;
    AtomicDMMContainer lastMessage; 
    READ_POLICY readPolicy;
    OVERWRITE_POLICY overwritePolicy;
    bool yields = true;
    std::string deviceName; 
    bool waitingOnPull = false; 
    int lastPush = 0; 
    int requestPush = 0;
}; 

struct ReaderBox
{

    public:
        std::unordered_map<DeviceID, DeviceBox> waitingQs;
        /* 
            Consists of the ordered list of all triggered events 
            to be queued up and sent to the execution manager
        */ 
        std::vector<EMStateMessage> triggerCache; 
        std::unordered_set<TaskID>& triggerSet; 
        TriggerManager triggerMan; 

        bool callbackRecived;
        bool statesRequested = false;
        std::string TaskName;
        bool pending_requests; 
        // used when forwarding packets to the EM when while the process is waiting for write permissions
        bool forwardPackets = false; 
        bool inExec = false;
        TaskDescriptor taskDesc; 
        std::mutex read_mut; 
        TSQ<EMStateMessage> &sendEM; 
        std::unordered_map<DeviceID, DeviceDescriptor> devDesc; 


        // Inserts the state into the object: 
        // the bool initEvent determines if the event is an initial event or not
        void insertState(HeapMasterMessage newDMM){
            std::lock_guard<std::mutex> lock(this->read_mut); 

            if(!this->waitingQs.contains(newDMM.info.device)){
                return; 
            }

            if(newDMM.protocol == PROTOCOLS::CALLBACKRECIEVED && (this->TaskName == newDMM.info.task) && this->devDesc.at(newDMM.info.device).ignoreWriteBacks){
                std::cout<<"Write back ignored"<<std::endl; 
                return; 
            }

            DeviceBox& targDev = this->waitingQs[newDMM.info.device];

            if((targDev.readPolicy == READ_POLICY::ANY) && inExec){
                //std::cout<<"Process in Exection! Abandoned device"<<std::endl;
                return;
            }

            if(forwardPackets && (targDev.readPolicy == READ_POLICY::ANY)){
                EMStateMessage ems; 
                //std::cout<<"FORWARDING MESSAGE for device: "<<newDMM.info.device<<std::endl;
                ems.protocol = PROTOCOLS::WAIT_STATE_FORWARD; 
                ems.dmm_list = {newDMM}; 
                sendEM.write(ems); 
                return; 
            }

            //std::cout<<"Inserting the state for: "<<newDMM.info.device<<std::endl;
            if(targDev.readPolicy == READ_POLICY::ANY){
                targDev.stateQueues->clearQueue(); 
            }
            targDev.stateQueues->write(newDMM); 
            targDev.lastMessage.replace(newDMM); 

            // Begin Trigger Analysis: 
            int triggerId = -2; 
            bool writeTrig = this->triggerMan.processDevice(newDMM.info.device, triggerId); 
    
            if(writeTrig){
                if(!this->forwardPackets){
                    this->triggerSet.insert(this->TaskName); 
                }
               
                std::string triggerName = "";
                uint16_t priority = 1;
                if (triggerId > -2) {
                    if(triggerId == -1){
                        // initial trigger
                        //std::cout<<"SETTING AN INIITAL TRIGGER"<<std::endl;  
                        triggerName = "initial";
                        priority = 1;
                    }
                    else{
                        auto& trigInfo = this->taskDesc.triggers.at(triggerId);
                        triggerName = trigInfo.id;
                        priority = trigInfo.priority;
                    }
                }
                std::vector<HeapMasterMessage> trigEvent; 
                
                for(auto& [name, devBox] : this->waitingQs){
                    
                    if(!devBox.stateQueues->isEmpty()){
                        auto newHmm = devBox.stateQueues->read(); 
                        newHmm.info.task = this->TaskName; 
                        newHmm.info.priority = priority; 
                        if(!devBox.stateQueues->isEmpty()){
                            int i = 0; 
                            this->triggerMan.processDevice(devBox.deviceName, i);
                        }
                        trigEvent.push_back(newHmm); 
                    }
                    else{
                        auto newHmm = devBox.lastMessage.get(); 
                        newHmm.info.task = this->TaskName; 
                        newHmm.info.priority = priority; 
                        trigEvent.push_back(newHmm);
                    }
                }

                EMStateMessage ems; 
                ems.dmm_list = trigEvent; 
                ems.TriggerName = triggerName;
                ems.priority = priority; 
                ems.taskName = this->TaskName; 
                ems.protocol = PROTOCOLS::SENDSTATES; 
            
                this->triggerCache.push_back(ems); 
            }
        }

        void handleRequest(){
            std::lock_guard<std::mutex> lock(this->read_mut); 

            // write for loop to handle requests
            if(pending_requests){
                int maxQueueSz = this->triggerCache.size() < MAX_EM_QUEUE_FILL?  triggerCache.size() : MAX_EM_QUEUE_FILL;     
                for(int i = 0; i < maxQueueSz; i++){
                    // Update when trigger naming comes 
                    if(!triggerCache.empty()){
                        auto ems = this->triggerCache.back(); 
                        this->triggerCache.pop_back(); 
                        sendEM.write(ems); 
                    }
                    else{
                        std::cerr<<"Strange: Max queue size is empty but claims size: "<<maxQueueSz<<std::endl; 
                    }
                }
            }   
        }   


        ReaderBox(std::string name,  TaskDescriptor& taskDesc, std::unordered_set<TaskID> &trigSet, TSQ<EMStateMessage> &emMsg);
    
};


// Holds confirmations based on the yeilding/callback policy
class ConfirmContainer{
 
    // Contains the currently held information about a device; 

    struct DeviceState{
        DynamicMasterMessage confirmDMM;
        // Cursors can store the pulls for confirmations

        bool waitingForCallback = false; 
        bool emptyQueue = true; 
        bool expectingYield = true; 
        bool pendingSend = false;         
        OVERWRITE_POLICY action = OVERWRITE_POLICY::NONE; 
    }; 

    // struct describing the actions that can be taking at a callback retrieval; 
    // NOTE: YIELD AND OVERWRITE POLICIES ARE MUTUALLY EXCLUSIVE
    struct TaskActionMetadata{
        bool yield = true; 
        OVERWRITE_POLICY policy; 
    }; 
    
    private: 
        TSQ<DynamicMasterMessage>& sendNM; 
        std::unordered_map<TaskID, std::unordered_map<DeviceID, TaskActionMetadata>> yieldPolicy; 
        std::unordered_map<DeviceID, DeviceState> loadedDevMap; 
 
        // Necessary as the Confirm container is accessed by the read and write blocks
        std::mutex mut; 

        void queueConfirmation(const DynamicMasterMessage &dmm, const std::string &devName){
            auto& state = this->loadedDevMap.at(devName);
            state.confirmDMM = dmm; 
            state.pendingSend = true; 
            auto& actionMetadata = this->yieldPolicy.at(dmm.info.task).at(dmm.info.device);

            state.expectingYield = actionMetadata.yield;
            
            if(!state.expectingYield){
               state.action = actionMetadata.policy; 
            }

        }

        bool attemptSendConfirm(DeviceState& state){
            if(state.pendingSend && !state.waitingForCallback){
                if(state.expectingYield){
                    if(state.emptyQueue){
                        this->sendNM.write(state.confirmDMM);
                        state.pendingSend = false; 
                        return true;
                    }
                }
                else{
                    this->sendNM.write(state.confirmDMM);
                     state.pendingSend = false; 
                     return true; 
                }
            }
            return false; 
        }

    public: 
        ConfirmContainer(TSQ<DynamicMasterMessage>& sendNM, std::vector<TaskDescriptor>& taskDescs)
        :sendNM(sendNM)  
        {
            for(auto& task : taskDescs){
                for(auto& devDesc : task.binded_devices){
                    this->yieldPolicy[task.name][devDesc.device_name].yield = devDesc.isYield; 
                    this->yieldPolicy[task.name][devDesc.device_name].policy = devDesc.overwritePolicy;
                  

                    if(!this->loadedDevMap.contains(devDesc.device_name)){
                        DeviceID dev = devDesc.device_name; 
                        if(devDesc.deviceKind == DeviceKind::CURSOR){
                            dev = devDesc.device_name + "::" + task.name; 
                        }
                        this->loadedDevMap.emplace(dev, DeviceState{}); 
                    }
                }
            }  
        }


        // Since yield and (OW::CLEAR/OW::)
        OVERWRITE_POLICY notifyRecievedCallback(DeviceID& dev){
            std::lock_guard<std::mutex> lock(this->mut);
            //std::cout<<"Notfied callback received"<<std::endl; 
            auto& state = loadedDevMap.at(dev);
            state.waitingForCallback = false; 
            if(attemptSendConfirm(state)){
                return state.action;
            }
            return OVERWRITE_POLICY::NONE; 
        }

        void notifySentMessage(DeviceID& dev){
          
            std::lock_guard<std::mutex> lock(this->mut);
            //std::cout<<"Notified sent message"<<std::endl; 
            auto& state = loadedDevMap.at(dev);
            state.waitingForCallback = true; 
            state.emptyQueue = false; 
        }

        // Means callback was received and 
        void notifyEmpty(DeviceID& dev){ 
         
            std::lock_guard<std::mutex> lock(this->mut);
            //std::cout<<"Notified empty queue"<<std::endl; 
            auto& state = loadedDevMap.at(dev);
            state.emptyQueue = true; 
            state.waitingForCallback = false; 
            attemptSendConfirm(state);
        }

        void send(const DynamicMasterMessage &dmm, TaskID ns = ""){
            std::lock_guard<std::mutex> lock(this->mut);
            DeviceID dev = dmm.info.device; 
            if(ns != ""){
                dev = dev + "::" + ns; 
            }
            auto& state = loadedDevMap.at(dev);
            if(state.pendingSend){
                throw std::runtime_error("Overlapping task confirmation requests, should not be happening!");
            }

            if(state.waitingForCallback){
                //std::cout<<"adding device "<<dmm.info.device<<" to queue to wait for callback"<<std::endl; 
                queueConfirmation(dmm, dev); 
                return; 
            }

            if(state.expectingYield || (dmm.protocol == PROTOCOLS::PULL_REQUEST)){
                //std::cout<<"Expecing yeild reached for device: "<<dmm.info.device<<std::endl; 
                if(state.emptyQueue){
                    //std::cout<<"Writing state: "<<dmm.info.device<<std::endl; 
                    this->sendNM.write(dmm);
                }
                else{
                    //std::cout<<"Adding the queue to the result: "<<dmm.info.device<<std::endl; 
                    queueConfirmation(dmm, dev);
                }
            }
            else{
                //std::cout<<"Sending straight out for device: "<<dmm.info.device<<std::endl; 
                this->sendNM.write(dmm);
            }
        } 
}; 

struct ManagedVType{
    ControllerQueue<SchedulerReq, ReqComparator> queue; 
    TaskID owner;
    bool isOwned = false;  

}; 


struct WriterBox
{
    private: 
       void procOverwrite(OVERWRITE_POLICY ow, TSQ<DynamicMasterMessage> &wQ){
            switch(ow){
                case(OVERWRITE_POLICY::CLEAR):{
                    wQ.clearQueue();               
                    break; 
                }
                case(OVERWRITE_POLICY::CURRENT):{
                    isFrozen = true; 
                    break; 
                }
                default:{
                    break; 
                }
            }
        }


    public:
    // Single Channel for Non-Cursor devices
    TSQ<DynamicMasterMessage> waitingQ;
    bool waitingForCallback = false;
    // Per-Task multi-channel map for task devices

    std::string deviceName;
    bool isFrozen = false;
    bool isCursor = false; 
    TSQ<DynamicMasterMessage> &sendNM; 
    ConfirmContainer &outHolder; 
    std::mutex m; 

    WriterBox(DeviceDescriptor& desc, TSQ<DynamicMasterMessage> &snm, ConfirmContainer &cc)
    : sendNM(snm), outHolder(cc)
    {
        this->isCursor = (desc.deviceKind == DeviceKind::CURSOR); 
        this->deviceName = desc.device_name; 
    }

    WriterBox(std::string deviceName, TSQ<DynamicMasterMessage> &dmm);



    void writeOut(HeapMasterMessage& hmm, OVERWRITE_POLICY ow, PROTOCOLS pcode){
        std::unique_lock<std::mutex> lock(m); 
        DynamicMasterMessage dmm = hmm.buildDMM(); 
        dmm.protocol = pcode; 
        // Logic for non-cursor devices
        switch(ow){
            case(OVERWRITE_POLICY::DISCARD):{
                if(waitingQ.isEmpty() && !waitingForCallback){
                    this->sendNM.write(dmm);
                }   
                break; 
            }
            case(OVERWRITE_POLICY::CURRENT):{
                this->sendNM.write(hmm.buildDMM()); 
                this->isFrozen = false;
                break; 
            }
            default:{
                if(!this->waitingForCallback){
                    this->sendNM.write(dmm); 
                    this->waitingForCallback = true; 
                }
                else{
                    this->waitingQ.write(dmm); 
                }
            }
        }
    
        outHolder.notifySentMessage(this->deviceName); 
    }


    void notifyCallBack(){
        std::unique_lock<std::mutex> lock(m); 
        if(!waitingQ.isEmpty()){
            OVERWRITE_POLICY ow  = this->outHolder.notifyRecievedCallback(this->deviceName); 
            procOverwrite(ow, this->waitingQ); 
            if(!isFrozen){
                this->sendNM.write(this->waitingQ.read());
            }
        }
        else{
            this->outHolder.notifyEmpty(this->deviceName); 
            this->waitingForCallback = false;

        }
    }
};





class MasterMailbox
{
    public:
    TSQ<DynamicMasterMessage> &readNM;
    TSQ<HeapMasterMessage> &readEM;
    TSQ<EMStateMessage> &sendEM;
    TSQ<DynamicMasterMessage> &sendNM;
    MasterMailbox(std::vector<TaskDescriptor> TaskList, TSQ<DynamicMasterMessage> &readNM, TSQ<HeapMasterMessage> &readEM,
         TSQ<DynamicMasterMessage> &sendNM, TSQ<EMStateMessage> &sendEM);
    std::vector<TaskDescriptor> TaskList;
    std::unordered_map<DeviceID, ControllerID> parentCont; 
    static DynamicMasterMessage buildDMM(HeapMasterMessage &hmm); 
    
    // List of tasks that were triggered (used to ensure intended-order execution for trigger groups)
    std::unordered_set<TaskID> triggerSet; 
    std::unordered_set<DeviceID> targetedDevices; 

    // number of found requests 
    int requestCount = 0;

    std::unordered_map<TaskID, std::unique_ptr<ReaderBox>> taskReadMap;
    std::unordered_map<DeviceID, std::unique_ptr<WriterBox>> deviceWriteMap;
    std::unordered_map<DeviceID, std::vector<TaskID>> interruptName_map;
    std::unordered_map<DeviceID, ManagedVType> vTypesSchedule; 
    ConfirmContainer ConfContainer; 


    TSQ<std::string> readRequest; 

    // Helper functions for sending items; 
    void notifyCallback(); 
    void notifyEmptyQueue();

    void assignNM(DynamicMasterMessage DMM);
    void assignEM(HeapMasterMessage DMM);
    void runningNM();
    void runningEM();
}; 

