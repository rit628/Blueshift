#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include "libtype/bls_types.hpp"
#include <algorithm>
#include <bitset>
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
        std::unordered_set<std::bitset<BITSET_SZ>> ruleset; 
        


    public: 
        // Device Constructor (created rules)
        TriggerManager(OBlockDesc& OBlockDesc){
            int i = 0; 
            for(DeviceDescriptor& devDesc : OBlockDesc.binded_devices){
                stringMap[devDesc.device_name] = i; 
                i++; 
            }

            // Add the default universal bitset (all 1s): 
            std::bitset<BITSET_SZ> defaultOption;
            defaultOption = (1 << (i)) - 1; 
            this->ruleset.insert(defaultOption); 

            // Loop through rules; 
            for(auto& data : OBlockDesc.triggers)
            {
                auto& rule = data.rule;
                std::bitset<BITSET_SZ> king; 
                for(auto& devName : rule){
                    king.set(this->stringMap[devName]); 
                }       
                ruleset.insert(king); 
            }
            
        }

        private: 
            // Tests bit against ruleset: 
            bool testBit(int& id){
                int i = 0; 
                for(auto& ruleBit : this->ruleset){
                    if((this->currentBitmap & ruleBit) == ruleBit){
                        id = i; 
                        return true;
                    }; 
                    i++; 
                }
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
    bool isTrigger = false;  
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
        std::vector<std::vector<HeapMasterMessage>> triggerCache; 
        TriggerManager triggerMan; 

        bool callbackRecived;
        bool dropRead = false;
        bool dropWrite = true;
        bool statesRequested = false;
        string OblockName;
        bool pending_requests; 
        // used when forwarding packets to the EM when while the process is waiting for write permissions
        bool forwardPackets = false; 


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
                std::vector<HeapMasterMessage> trigEvent; 
                for(auto& [name, devBox] : this->waitingQs){
                    if(devBox.stateQueues->isEmpty()){
                        auto newHmm = devBox.stateQueues->read(); 
                        newHmm.info.oblock = this->OblockName; 
                        trigEvent.push_back(newHmm); 
                    }
                    else{
                        auto newHmm = devBox.lastMessage.get(); 
                        newHmm.info.oblock = this->OblockName; 
                        trigEvent.push_back(newHmm);
                    }
                }

                this->triggerCache.push_back(trigEvent); 
            }
        }

        void handleRequest(TSQ<vector<HeapMasterMessage>>& sendEM){
            // write for loop to handle requests
            if(pending_requests){
                int maxQueueSz = this->triggerCache.size() < MAX_EM_QUEUE_FILL?  triggerCache.size() : MAX_EM_QUEUE_FILL;     
                for(int i = 0; i < maxQueueSz; i++){
                    auto dmmVect = this->triggerCache.back(); 
                    // Update when trigger naming comes 
                    this->triggerCache.pop_back(); 
                    EMStateMessage ems; 
                    ems.TriggerName = "NONE"; 
                    ems.dmm_list = dmmVect; 
                    ems.protocol = PROTOCOLS::SENDSTATES; 
                    ems.priority = 1; 
                    ems.oblockName = this->OblockName; 
                    sendEM.write(ems); 
                }
            }   
        }   

        ReaderBox(bool dropRead, bool dropWrite, string name,  OBlockDesc& odesc);
    
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
    TSQ<vector<HeapMasterMessage>> &sendEM;
    TSQ<DynamicMasterMessage> &sendNM;
    MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, TSQ<HeapMasterMessage> &readEM,
         TSQ<DynamicMasterMessage> &sendNM, TSQ<vector<HeapMasterMessage>> &sendEM);
    vector<OBlockDesc> OBlockList;

    unordered_map<OblockID, unique_ptr<ReaderBox>> oblockReadMap;
    unordered_map<DeviceID, unique_ptr<WriterBox>> deviceWriteMap;
    unordered_map<DeviceID, std::vector<OblockID>> interruptName_map;

    TSQ<std::string> readRequest; 

    void assignNM(DynamicMasterMessage DMM);
    void assignEM(HeapMasterMessage DMM);
    void runningNM();
    void runningEM();
}; 