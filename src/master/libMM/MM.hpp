#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include <bitset>
#include <mutex>
#include <unordered_map>
#include <map>
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
        DynamicMasterMessage dmsg; 
        bool hasItem = false; 
        std::mutex mux; 
    
    public: 
        void replace(DynamicMasterMessage new_dmsg){
            std::scoped_lock<std::mutex> lock(mux); 
            hasItem = true; 
            this->dmsg = new_dmsg; 
        }

        DynamicMasterMessage get(){
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
            for(DeviceDescriptor& devDesc : OBlockDesc.inDevices){
                if(!devDesc.isVtype){
                    stringMap[devDesc.device_name] = i; 
                    i++; 
                }
            }

            // Add the default universal bitset (all 1s): 
            std::bitset<BITSET_SZ> defaultOption;
            defaultOption = (1 << (i)) - 1; 
            this->ruleset.insert(defaultOption); 

            // Loop through rules; 
            for(auto& rule : OBlockDesc.triggerRules){
                std::bitset<BITSET_SZ> king; 
                for(auto& devName : rule){
                    king.set(this->stringMap[devName]); 
                }       
                ruleset.insert(king); 
            }
            
        }

        private: 
            // Tests bit against ruleset: 
            bool testBit(){
                return std::ranges::any_of(this->ruleset.begin(), this->ruleset.end(), 
                    [this](const auto& val){
                        return (this->currentBitmap & val) == val;
                    }
                );
            }
        public: 
            // Returns true if the new device corresponds to a trigger 
            bool processDevice(std::string &object){
                // Pretty much check if the bitmap is filled
                std::cout<<"What the hell boy"<<std::endl; 
                int filledMap = pow(2, stringMap.size()) - 1; 
                std::cout<<"map value: "<<filledMap<<std::endl; 
                if(this->initBitmap.to_ulong() != filledMap){
                    this->initBitmap.set(this->stringMap[object]); 
                    std::cout<<"init bitmap: "<<this->initBitmap<<std::endl; 
                    return false; 
                }

                this->currentBitmap.set(this->stringMap[object]);         
                bool found = this->testBit(); 
                std::cout<<"KING KING KING"<<std::endl; 
                if(found){
                    this->currentBitmap.reset(); 
                    std::cout<<"I LOVE KIIIIIIDS"<<std::endl; 
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
    std::shared_ptr<TSQ<DynamicMasterMessage>> stateQueues;
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
        std::vector<std::vector<DynamicMasterMessage>> triggerCache; 
        TriggerManager triggerMan; 

        bool callbackRecived;
        bool dropRead = false;
        bool dropWrite = true;
        bool statesRequested = false;
        string OblockName;
        bool pending_requests; 


    
        // Inserts the state into the object: 
        // the bool initEvent determines if the event is an initial event or not
        void insertState(DynamicMasterMessage newDMM){
            if(!this->waitingQs.contains(newDMM.info.device)){
                return; 
            }
            DeviceBox& targDev = this->waitingQs[newDMM.info.device];
            if(targDev.devDropRead){
                targDev.stateQueues->clearQueue(); 
            }
            targDev.stateQueues->write(newDMM); 
            targDev.lastMessage.replace(newDMM); 


            // Begin Trigger Analysis: 
            bool writeTrig = this->triggerMan.processDevice(newDMM.info.device); 
            if(writeTrig){
                std::vector<DynamicMasterMessage> trigEvent; 
                for(auto& [name, devBox] : this->waitingQs){
                    if(devBox.stateQueues->isEmpty()){
                        trigEvent.push_back(devBox.stateQueues->read()); 
                    }
                    else{
                        trigEvent.push_back(devBox.lastMessage.get());
                    }
                }
                this->triggerCache.push_back(trigEvent); 
            }
        }

        void handleRequest(TSQ<vector<DynamicMasterMessage>>& sendEM){
            // write for loop to handle requests
            if(pending_requests){
                int maxQueueSz = this->triggerCache.size() < MAX_EM_QUEUE_FILL?  triggerCache.size() : MAX_EM_QUEUE_FILL;     
                for(int i = 0; i < maxQueueSz; i++){
                    auto dmm = this->triggerCache.back(); 
                    this->triggerCache.pop_back(); 
                    sendEM.write(dmm); 
                }
            }   
        }   

        ReaderBox(bool dropRead, bool dropWrite, string name,  OBlockDesc& odesc);
        ReaderBox() = default;
    
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
    TSQ<DynamicMasterMessage> &readEM;
    TSQ<DynamicMasterMessage> &sendNM;
    TSQ<vector<DynamicMasterMessage>> &sendEM;
    MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, TSQ<DynamicMasterMessage> &readEM,
         TSQ<DynamicMasterMessage> &sendNM, TSQ<vector<DynamicMasterMessage>> &sendEM);
    vector<OBlockDesc> OBlockList;

    unordered_map<OblockID, unique_ptr<ReaderBox>> oblockReadMap;
    unordered_map<DeviceID, unique_ptr<WriterBox>> deviceWriteMap;
    unordered_map<DeviceID, std::vector<OblockID>> interruptName_map;


    
    TSQ<std::string> readRequest; 



    void assignNM(DynamicMasterMessage DMM);
    void assignEM(DynamicMasterMessage DMM);
    void runningNM();
    void runningEM();
}; 