#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include "libtype/bls_types.hpp"
#include <algorithm>
#include <bitset>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>



using namespace std;

using OblockID = std::string;
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
        std::set<int> excludedTriggers;

        


    public: 
        // Device Constructor (created rules)
        TriggerManager(OBlockDesc& OBlockDesc){
            int i = 0; 
            for(DeviceDescriptor& devDesc : OBlockDesc.binded_devices){
                stringMap[devDesc.device_name] = i; 
                i++; 
            }

            // Add the default universal bitset (all 1s): 
            defaultBitstring = (1 << (i)) - 1; 
            this->trigData = OBlockDesc.triggers; 

            // Loop through rules; 
            for(auto& data : OBlockDesc.triggers)
            {
                auto& rule = data.rule;
                std::bitset<BITSET_SZ> king; 
                for(auto& devName : rule){
                    king.set(this->stringMap[devName]); 
                }       
                ruleset.push_back(king); 
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
                    }; 
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
}; 


struct DeviceBox{
    std::shared_ptr<TSQ<HeapMasterMessage>> stateQueues;
    AtomicDMMContainer lastMessage; 
    READ_POLICY readPolicy;
    OVERWRITE_POLICY overwritePolicy;
    bool yields = true;
    std::string deviceName; 
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
        std::unordered_set<OblockID>& triggerSet; 
        TriggerManager triggerMan; 

        bool callbackRecived;
        bool statesRequested = false;
        string OblockName;
        bool pending_requests; 
        // used when forwarding packets to the EM when while the process is waiting for write permissions
        bool forwardPackets = false; 
        bool inExec = false;
        OBlockDesc oblockDesc; 


        // Inserts the state into the object: 
        // the bool initEvent determines if the event is an initial event or not
        void insertState(HeapMasterMessage newDMM, TSQ<EMStateMessage>& sendEM){
            if(!this->waitingQs.contains(newDMM.info.device)){
                return; 
            }

            DeviceBox& targDev = this->waitingQs[newDMM.info.device];

            if((targDev.readPolicy == READ_POLICY::ANY) && inExec){
                std::cout<<"Process in Exection! Abandoned device"<<std::endl;
                return;
            }


            if(forwardPackets && targDev.yields){
                EMStateMessage ems; 
                std::cout<<"FORWARDING MESSAGE for device: "<<newDMM.info.device<<std::endl;
                ems.protocol = PROTOCOLS::WAIT_STATE_FORWARD; 
                ems.dmm_list = {newDMM}; 
                sendEM.write(ems); 
                return; 
            }

            std::cout<<"Inserting the state"<<std::endl;
            targDev.stateQueues->write(newDMM); 
            targDev.lastMessage.replace(newDMM); 

            // Begin Trigger Analysis: 
            int triggerId = -1; 
            bool writeTrig = this->triggerMan.processDevice(newDMM.info.device, triggerId); 
    
            if(writeTrig){
                if(!this->forwardPackets){
                    this->triggerSet.insert(this->OblockName); 
                }
               
                auto& trigInfo = this->oblockDesc.triggers.at(triggerId);
                std::vector<HeapMasterMessage> trigEvent; 
                
                for(auto& [name, devBox] : this->waitingQs){
                    if(devBox.stateQueues->isEmpty()){
                        auto newHmm = devBox.stateQueues->read(); 
                        newHmm.info.oblock = this->OblockName; 
                        newHmm.info.priority = trigInfo.priority; 
                        trigEvent.push_back(newHmm); 
                    }
                    else{
                        auto newHmm = devBox.lastMessage.get(); 
                        newHmm.info.oblock = this->OblockName; 
                        newHmm.info.priority = trigInfo.priority; 
                        trigEvent.push_back(newHmm);
                    }
                }

                EMStateMessage ems; 
                ems.dmm_list = trigEvent; 
                ems.TriggerName = trigInfo.id;  
                ems.priority = trigInfo.priority; 
                ems.oblockName = this->OblockName; 
                ems.protocol = PROTOCOLS::SENDSTATES; 
            
                this->triggerCache.push_back(ems); 
            }
        }

        void handleRequest(TSQ<EMStateMessage>& sendEM){
            // write for loop to handle requests
            if(pending_requests){
                int maxQueueSz = this->triggerCache.size() < MAX_EM_QUEUE_FILL?  triggerCache.size() : MAX_EM_QUEUE_FILL;     
                for(int i = 0; i < maxQueueSz; i++){
                    auto dmmVect = this->triggerCache.back(); 
                    // Update when trigger naming comes 
                    auto ems = this->triggerCache.back(); 
                    this->triggerCache.pop_back(); 
                    sendEM.write(ems); 
                }
            }   
        }   


        ReaderBox(string name,  OBlockDesc& odesc, std::unordered_set<OblockID> &trigSet);
    
};





struct WriterBox
{
    public:
    TSQ<DynamicMasterMessage> waitingQ;
    bool waitingForCallback = false;
    string deviceName;
    bool isFrozen = false; 
    
    WriterBox(string deviceName);
    WriterBox() = default;

};


// Holds confirmations based on the yeilding/callback policy
class ConfirmContainer{
 
    // Contains the currently held information about a device; 
    struct DeviceState{
        DynamicMasterMessage confirmDMM;
        bool waitingForCallback = false; 
        bool emptyQueue = true; 
        bool expectingYield = true; 
        bool pendingSend = false;         
        OVERWRITE_POLICY action = OVERWRITE_POLICY::NONE; 

    }; 

    // struct describing the actions that can be taking at a callback retrieval; 
    // NOTE: YIELD AND OVERWRITE POLICIES ARE MUTUALLY EXCLUSIVE
    struct OblockActionMetadata{
        bool yield = true; 
        OVERWRITE_POLICY policy; 
    }; 
    
    private: 
        TSQ<DynamicMasterMessage>& sendNM; 
        std::unordered_map<OblockID, std::unordered_map<DeviceID, OblockActionMetadata>> yieldPolicy; 
        std::unordered_map<DeviceID, DeviceState> loadedDevMap; 

        void queueConfirmation(const DynamicMasterMessage &dmm){
            auto& state = this->loadedDevMap.at(dmm.info.device);
            state.confirmDMM = dmm; 
            state.pendingSend = true; 
            auto& actionMetadata = this->yieldPolicy.at(dmm.info.oblock).at(dmm.info.device);

            state.expectingYield = actionMetadata.yield;
            
            if(!state.expectingYield){
               state.action = actionMetadata.policy; 
            }

        }

        bool attemptSendConfirm(DeviceState& state){
            if(state.pendingSend && !state.waitingForCallback){
                if(state.expectingYield){
                    if(state.emptyQueue){
                        std::cout<<"SENDING STATE EMPTY"<<std::endl; 
                        this->sendNM.write(state.confirmDMM);
                        state.pendingSend = false; 
                        return true;
                    }
                }
                else{
                    std::cout<<"SENDING STATE NORMAL"<<std::endl; 
                    this->sendNM.write(state.confirmDMM);
                     state.pendingSend = false; 
                     return true; 
                }
            }
            return false; 
        }

    public: 
        ConfirmContainer(TSQ<DynamicMasterMessage>& sendNM, std::vector<OBlockDesc>& oblockDescs)
        :sendNM(sendNM)  
        {
            for(auto& oblock : oblockDescs){
                for(auto& devDesc : oblock.binded_devices){
                    this->yieldPolicy[oblock.name][devDesc.device_name].yield = devDesc.isYield; 
                    this->yieldPolicy[oblock.name][devDesc.device_name].policy = devDesc.overwritePolicy;
                    if(!this->loadedDevMap.contains(devDesc.device_name)){
                        this->loadedDevMap.emplace(devDesc.device_name, DeviceState{}); 
                    }
                }
            }  
        }


        // Since yield and (OW::CLEAR/OW::)
        OVERWRITE_POLICY notifyRecievedCallback(DeviceID& dev){
            std::cout<<"Notfied callback received"<<std::endl; 
            auto& state = loadedDevMap.at(dev);
            state.waitingForCallback = false; 
            if(attemptSendConfirm(state)){
                return state.action;
            }
            return OVERWRITE_POLICY::NONE; 
        }

        void notifySentMessage(DeviceID& dev){
            std::cout<<"Notified sent message"<<std::endl; 
            auto& state = loadedDevMap.at(dev);
            state.waitingForCallback = true; 
            state.emptyQueue = false; 
        }

        // Means callback was received and 
        void notifyEmpty(DeviceID& dev){
            std::cout<<"Notified empty queue"<<std::endl; 
            auto& state = loadedDevMap.at(dev);
            state.emptyQueue = true; 
            state.waitingForCallback = false; 
            attemptSendConfirm(state);
        }

        void send(const DynamicMasterMessage &dmm){
            auto& state = loadedDevMap.at(dmm.info.device);
            if(state.pendingSend){
                throw std::runtime_error("Overlapping oblock confirmation requests, should not be happening!");
            }

            if(state.waitingForCallback){
                std::cout<<"adding to queue to wait for callback"<<std::endl; 
                queueConfirmation(dmm); 
                return; 
            }

            if(state.expectingYield){
                std::cout<<"Expecing yeild reached"<<std::endl; 
                if(state.emptyQueue){
                    std::cout<<"Writing state"<<std::endl; 
                    this->sendNM.write(dmm);
                }
                else{
                    std::cout<<"Adding the queue to the result"<<std::endl; 
                    queueConfirmation(dmm);
                }
            }
            else{
                this->sendNM.write(dmm);
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
    MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, TSQ<HeapMasterMessage> &readEM,
         TSQ<DynamicMasterMessage> &sendNM, TSQ<EMStateMessage> &sendEM);
    vector<OBlockDesc> OBlockList;
    unordered_map<DeviceID, ControllerID> parentCont; 
    static DynamicMasterMessage buildDMM(HeapMasterMessage &hmm); 
    
    // List of oblocks that were triggered (used to ensure intended-order execution for trigger groups)
    std::unordered_set<OblockID> triggerSet; 
    std::unordered_set<DeviceID> targetedDevices; 

    // number of found requests 
    int requestCount = 0;

    unordered_map<OblockID, unique_ptr<ReaderBox>> oblockReadMap;
    unordered_map<DeviceID, unique_ptr<WriterBox>> deviceWriteMap;
    unordered_map<DeviceID, std::vector<OblockID>> interruptName_map;
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