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
    unordered_map<string, unordered_map<string, shared_ptr<TSQ<DynamicMasterMessage>>>> readMap;
    unordered_map<string, unique_ptr<WriterBox>> writeMap;
    void assignNM(DynamicMasterMessage DMM);
    void assignEM(DynamicMasterMessage DMM);
    void runningNM();
    void runningEM();
};

