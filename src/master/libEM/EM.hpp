#pragma once

#include "../libTSQ/TSQ.hpp"
#include "../libTSM/TSM.hpp"
#include "../libDM/DynamicMessage.hpp"
#include "../libCommon/Common.hpp"
//#include "../../lang/libinterpreter/interpreter.hpp"
//#include "../../lang/common/ast.hpp"
#include <unordered_map>
#include <thread>
#include <memory>
#include <string>
#include <vector>
using namespace std;


class ExecutionUnit
{
    public:
    string OblockName;
    O_Info info;
    unordered_map<string, DynamicMasterMessage> stateMap;
    vector<string> devices;
    vector<bool> isVtype;
    vector<string> controllers;
    TSQ<vector<DynamicMasterMessage>> EUcache;
    thread executionThread;
    bool stop = false;
    ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers, 
        TSM<string, HeapMasterMessage> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);
    void running(TSM<string, HeapMasterMessage> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);
    vector<shared_ptr<HeapDescriptor>> transformState(vector<shared_ptr<HeapDescriptor>> HMM_List);
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, TSQ<DynamicMasterMessage> &sendMM);
    ExecutionUnit &assign(DynamicMasterMessage DMM);
    void running(TSQ<vector<DynamicMasterMessage>> &readMM);
    TSQ<vector<DynamicMasterMessage>> &readMM;
    TSQ<DynamicMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
    vector<HeapMasterMessage> vtypeHMMs;
    TSM<string, HeapMasterMessage> vtypeHMMsMap;
    //BlsLang::Interpreter interpreter;
    //BlsLang::AstNode ast;
};

