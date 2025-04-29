#pragma once

#include "include/Common.hpp"
#include "libTSQ/TSQ.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include <mutex>
#include <unordered_map>
#include <map>
#include <memory>
#include <string>
#include <vector>
using namespace std;

using OblockID = std::string;
using DeviceID = std::string; 
 
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
    bool callbackRecived;
    bool dropRead;
    bool dropWrite;
    bool statesRequested = false;
    string OblockName;
    bool pending_requests; 

    void handleRequest(TSQ<vector<DynamicMasterMessage>>& sendEM){
        // write for loop to handle requests
        if(pending_requests){
            int smallestTSQ = 10;
            bool hasTrigger = false; 
            bool lastStateWait = false; 

            std::vector<DynamicMasterMessage> statesToSend; 
            for(auto& pair : waitingQs){
                auto& devBox = pair.second; 
                if(devBox.isTrigger){
                    hasTrigger = true; 
                    if((devBox.stateQueues->getSize() < smallestTSQ)){
                        //std::cout<<"Corresponding to device: "<<devBox.deviceName<<std::endl; 
                        //std::cout<<"Has state queue: "<<devBox.stateQueues->getSize()<<std::endl; 
                        smallestTSQ = devBox.stateQueues->getSize();
                    }
                }
                else{
                    if(!devBox.lastMessage.isInit()){
                        lastStateWait = true; 
                        //std::cout<<"Waiting for a last message from a persistent state device"<<std::endl; 
                        return; 
                    }
                }
            }

            //std::cout<<"Writing states to with trigger size: "<<smallestTSQ<<std::endl;  

            if((smallestTSQ > 0) && hasTrigger){
                for(int i = 0; i < smallestTSQ; i++){
                    std::vector<DynamicMasterMessage> statesToSend; 
                    for(auto& pair : waitingQs){
                        auto& devBox = pair.second; 
                        if(devBox.isTrigger){
                            auto dmm = devBox.stateQueues->read(); 
                            statesToSend.push_back(dmm); 
                        }
                        else{
                            statesToSend.push_back(devBox.lastMessage.get()); 
                        }
                    }
                    sendEM.write(statesToSend); 
                }                
            }
            else{
                std::cout<<"Waiting on a trigger device"<<std::endl; 
            }
        }   
    }   

    ReaderBox(bool dropRead, bool dropWrite, string name);
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