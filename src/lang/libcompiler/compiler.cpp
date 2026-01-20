#include "compiler.hpp"
#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>
#include <boost/range/combine.hpp>
#include <variant>
#include <vector>


using namespace BlsLang;

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
    ast->accept(generator);
    if (auto* stream = std::get_if<std::reference_wrapper<std::vector<char>>>(&outputStream)) {
        generator.writeBytecode(*stream);
    }
    else {
        generator.writeBytecode(std::get<std::reference_wrapper<std::ostream>>(outputStream));
    }
    ast->accept(this->depGraph);
    // auto tempTask = this->depGraph.getTaskMap();  


    /*
    this->symGraph.setMetadata(this->depGraph.getTaskMap(), analyzer.getTaskDescriptors()); 
    ast->accept(this->symGraph); 
    this->symGraph.annotateControllerDivide(); 

    auto divider = this->symGraph.getDivisionData(); 

    // Verify that the deviders work!
    for(auto& obj : divider.ctlMetaData){
        std::cout<<"Printing data for controller ID: "<<obj.first<<std::endl; 
        for(auto data : obj.second.taskData){
            std::cout<<"Original task: "<<data.first<<std::endl; 
            std::cout<<"Params: "<<std::endl;
            for(auto param : data.second.parameterList){
                std::cout<<param<<" "; 
            }
            std::cout<<"\n"; 
            std::cout<<"Jamar device"<<std::endl; 
            for(auto jamar : data.second.taskDesc.binded_devices){
                std::cout<<jamar.device_name<<" "; 
            }
            std::cout<<"\n"; 
        }
    }
    
    // Setting up the divider
    this->divider.setMetadata(divider); 
    ast->accept(this->divider);

    auto& king = this->divider.getControllerSplit(); 
    std::cout<<"Split ctl\n"<<std::endl; 
    for(auto& hom : king){
        std::cout<<"Controller: "<<hom.first<<std::endl; 
        auto& cntSrc = hom.second; 
        for(auto& desc : cntSrc.taskDesc){
            std::cout<<"\ntaskname: "<<desc.first<<std::endl; 
            std::cout<<"Internal name:"<<desc.second.name<<std::endl; 
            for(auto& dev : desc.second.binded_devices){
                std::cout<<"Binded: "<<dev.device_name<<std::endl; 
            }
        }   
    }

    std::cout<<"\nend Split ctl"<<std::endl;
    */ 

    ast->accept(masterInterpreter);
        
    auto& descriptors = analyzer.getTaskDescriptors();

    auto& masterTasks = masterInterpreter.getTasks();
    euInterpreters.assign(descriptors.size(), masterInterpreter);
    for (auto&& [descPair, interpreter] : boost::combine(descriptors, euInterpreters)) {
        auto& [taskName, descriptor] = descPair;
        if (!masterTasks.contains(taskName)) { continue; }
        auto& task = masterTasks.at(taskName);
        tasks.emplace(taskName, [&task, &interpreter](std::vector<BlsType> v) { return task(interpreter, v); });
        taskDescriptors.push_back(descriptor);
    }

}