#pragma once

#include "libTSQ/TSQ.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libEM/EM.hpp"
#include <unordered_map>
#include <map>
#include <memory>
#include <string>
#include <vector>
using namespace std;

struct ReaderBox
{
    public:
    vector<shared_ptr<TSQ<DynamicMasterMessage>>> waitingQs;
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
            std::vector<DynamicMasterMessage> statesToSend; 
            for(int i = 0; i < this->waitingQs.size(); i++)
                {
                TSQ<DynamicMasterMessage> &currentTSQ = *this->waitingQs.at(i);
                if(currentTSQ.getSize() <= smallestTSQ){smallestTSQ = currentTSQ.getSize();}
                }
            for(int i = 0; i < smallestTSQ; i++)
                {
                    vector<DynamicMasterMessage> statesToSend;
                    for(int j = 0; j < this->waitingQs.size(); j++)
                    {
                        // REPLACE WITH POP ONCE THE DEPENDENCY GRAPH IS CREATED
                        statesToSend.push_back(this->waitingQs.at(j)->read());
                    }
                    sendEM.write(statesToSend);
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
    unordered_map<string, unique_ptr<ReaderBox>> oblockReadMap;
    unordered_map<string, vector<shared_ptr<TSQ<DynamicMasterMessage>>>> interruptMap;
    unordered_map<string, std::vector<string>> interruptName_map;
    unordered_map<string, unordered_map<string, shared_ptr<TSQ<DynamicMasterMessage>>>> readTSQMap;
    unordered_map<string, unique_ptr<WriterBox>> writeMap;
    TSQ<std::string> readRequest; 



    void assignNM(DynamicMasterMessage DMM);
    void assignEM(DynamicMasterMessage DMM);
    void runningNM();
    void runningEM();
};

