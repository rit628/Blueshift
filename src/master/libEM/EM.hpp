#pragma once

#include "libTSQ/TSQ.hpp"
#include "libTSM/TSM.hpp"
#include "libDM/DynamicMessage.hpp"
#include "include/Common.hpp"
#include "../../lang/libinterpreter/interpreter.hpp"
#include "../../lang/common/ast.hpp"
#include <unordered_map>
#include <thread>
#include <memory>
#include <string>
#include <vector>
#include <functional>
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
        TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, 
        function<vector<BlsLang::BlsType>(vector<BlsLang::BlsType>)>  transform_function);
    void running( TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);
    //vector<shared_ptr<HeapDescriptor>> transformState(vector<shared_ptr<HeapDescriptor>> HMM_List);
    function<vector<BlsLang::BlsType>(vector<BlsLang::BlsType>)>  transform_function;
    ~ExecutionUnit();
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, 
        TSQ<DynamicMasterMessage> &sendMM, 
        std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks);
    ExecutionUnit &assign(DynamicMasterMessage DMM);
    void running();
    TSQ<vector<DynamicMasterMessage>> &readMM;
    TSQ<DynamicMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
    //vector<HeapMasterMessage> vtypeHMMs;
    TSM<string, vector<HeapMasterMessage>> vtypeHMMsMap;
};

