#include "compiler.hpp"
#include "Serialization.hpp"
#include "depgraph.hpp"
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>
#include <boost/range/combine.hpp>
#include <unordered_map>
#include <variant>
#include <vector>


using namespace BlsLang;

void Compiler::modifyTaskDesc(std::unordered_map<std::string, TaskDescriptor> &oDescs,  GlobalContext &gcx){
    std::unordered_map<DeviceID, DeviceDescriptor&> devDesc; 
    for(auto& [name, task] : oDescs){
    
        std::unordered_map<DeviceID, DeviceDescriptor> devMap; 
        for(auto& str : task.binded_devices){
            devMap[str.device_name] = str; 
        }
        auto& inMap = gcx.taskConnections[task.name].inDeviceList; 
        auto& outMap = gcx.taskConnections[task.name].outDeviceList; 

        for(auto &devId : inMap){
            task.inDevices.push_back(devId);
        }

        for(auto& devId : outMap){
            task.outDevices.push_back(devId); 
        }   
    }
}


void Compiler::compileFile(const std::string& source, ostream_t outputStream) {
    std::ifstream file;
    file.open(source);
    if (!file.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    std::stringstream ss;
    ss << file.rdbuf();
    compileSource(ss.str(), outputStream);
}

void Compiler::compileSource(const std::string& source, ostream_t outputStream) {
    tokens = lexer.lex(source);
    ast = parser.parse(tokens);
    ast->accept(analyzer);
    ast->accept(depGraph);
    modifyTaskDesc(analyzer.getTaskDescriptors(), depGraph.getGlobalContext()); 
    ast->accept(generator);
    if (auto* stream = std::get_if<std::reference_wrapper<std::vector<char>>>(&outputStream)) {
        generator.writeBytecode(*stream);
    }
    else {
        generator.writeBytecode(std::get<std::reference_wrapper<std::ostream>>(outputStream));
    }

    ast->accept(masterInterpreter);
        
    auto& descriptors = analyzer.getBoundTasks();

    auto& masterTasks = masterInterpreter.getTasks();
    euInterpreters.assign(descriptors.size(), masterInterpreter);
    for (auto&& [taskDescriptor, interpreter] : boost::combine(descriptors, euInterpreters)) {
        if (!masterTasks.contains(taskDescriptor.name)) { continue; }
        auto& task = masterTasks.at(taskDescriptor.name);
        tasks.emplace(taskDescriptor.name, [&task, &interpreter](std::vector<BlsType> v) { return task(interpreter, v); });
    }

}