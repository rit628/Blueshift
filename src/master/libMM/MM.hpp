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
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <vector>



using namespace std;

using OblockID = std::string;
using DeviceID = std::string; 

# define MAX_EM_QUEUE_FILL 10 
 
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

#define BITSET_SZ 32


class TriggerManager{
    private:   
        std::bitset<BITSET_SZ> currentBitmap;  
        std::bitset<BITSET_SZ> initBitmap; 
        std::unordered_map<std::string, int> stringMap; 
        std::vector<std::bitset<BITSET_SZ>> ruleset; 
        std::vector<TriggerData> trigData; 
        std::bitset<BITSET_SZ> defaultBitstring; 
        


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
    bool devDropRead = false; 
    bool devDropWrite = false; 
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
        bool dropRead = false;
        bool dropWrite = true;
        bool statesRequested = false;
        string OblockName;
        bool pending_requests; 
        // used when forwarding packets to the EM when while the process is waiting for write permissions
        bool forwardPackets = false; 
        OBlockDesc oblockDesc; 


        // Inserts the state into the object: 
        // the bool initEvent determines if the event is an initial event or not
        void insertState(HeapMasterMessage newDMM, TSQ<EMStateMessage>& sendEM){
            if(!this->waitingQs.contains(newDMM.info.device)){
                return; 
            }

            DeviceBox& targDev = this->waitingQs[newDMM.info.device];

            if(forwardPackets && targDev.devDropRead){
                EMStateMessage ems; 
                ems.protocol = PROTOCOLS::WAIT_STATE_FORWARD; 
                ems.dmm_list = {newDMM}; 
                sendEM.write(ems); 
                return; 
            }

            if(targDev.devDropRead){
                targDev.stateQueues->clearQueue(); 
            }
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
    WriterBox(string deviceName);
    WriterBox() = default;
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


    TSQ<std::string> readRequest; 

    void assignNM(DynamicMasterMessage DMM);
    void assignEM(HeapMasterMessage DMM);
    void runningNM();
    void runningEM();
}; 