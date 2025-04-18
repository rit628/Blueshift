#pragma once

#include "libTSQ/TSQ.hpp"
#include "libTSM/TSM.hpp"
#include "libDM/DynamicMessage.hpp"
#include "include/Common.hpp"
#include "../../lang/libinterpreter/interpreter.hpp"
#include "../../lang/common/ast.hpp"
#include "libtypes/bls_types.hpp"
#include "../../lang/libvirtual_machine/virtual_machine.hpp"
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
    int bytecodeOffset = 0;
    BlsLang::VirtualMachine vm; 

    ExecutionUnit(string OblockName, vector<string> devices, vector<bool> isVtype, vector<string> controllers, 
        TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM, int offset);

    void running( TSM<string, vector<HeapMasterMessage>> &vtypeHMMsMap, TSQ<DynamicMasterMessage> &sendMM);
    ~ExecutionUnit();
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<DynamicMasterMessage>> &readMM, 
        TSQ<DynamicMasterMessage> &sendMM);

    ExecutionUnit &assign(DynamicMasterMessage DMM);
    void running();
    TSQ<vector<DynamicMasterMessage>> &readMM;
    TSQ<DynamicMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
    //vector<HeapMasterMessage> vtypeHMMs;
    TSM<string, vector<HeapMasterMessage>> vtypeHMMsMap;
};

