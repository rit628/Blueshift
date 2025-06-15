#pragma once

#include "libTSQ/TSQ.hpp"
#include "include/Common.hpp"
#include "../../lang/libinterpreter/interpreter.hpp"
#include "libScheduler/scheduler.hpp"
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
    OBlockDesc Oblock;
    O_Info info;
    unordered_map<string, HeapMasterMessage> stateMap;
    vector<string> devices;
    vector<bool> isVtype;
    vector<string> controllers;
    TSQ<vector<HeapMasterMessage>> EUcache;
    thread executionThread;
    bool stop = false;
    std::unordered_map<std::string, int> devicePositionMap; 

    ExecutionUnit(OBlockDesc OblockData, vector<string> devices, vector<bool> isVtype, vector<string> controllers, TSQ<HeapMasterMessage> &sendMM, function<vector<BlsType>(vector<BlsType>)>  transform_function);
    void running( TSQ<HeapMasterMessage> &sendMM);
   
    function<vector<BlsType>(vector<BlsType>)>  transform_function;
    ~ExecutionUnit();
};

class ExecutionManager
{
    public:
    //ExecutionManager() = default;
    ExecutionManager(vector<OBlockDesc> OblockList, TSQ<vector<HeapMasterMessage>> &readMM, 
        TSQ<HeapMasterMessage> &sendMM, 
        std::unordered_map<std::string, std::function<std::vector<BlsType>(std::vector<BlsType>)>> oblocks);

    ExecutionUnit &assign(HeapMasterMessage DMM);

    void running();

    TSQ<vector<HeapMasterMessage>> &readMM;
    TSQ<HeapMasterMessage> &sendMM;
    unordered_map<string, unique_ptr<ExecutionUnit>> EU_map;
    vector<OBlockDesc> OblockList;
};

