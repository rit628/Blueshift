#include "compiler.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>
#include <boost/range/combine.hpp>


using namespace BlsLang;

void Compiler::compileFile(const std::string& source, std::ostream& outputStream) {
    std::ifstream file;
    file.open(source);
    if (!file.is_open()) {
        throw std::runtime_error("Invalid Filename.");
    }
    std::stringstream ss;
    ss << file.rdbuf();
    compileSource(ss.str(), outputStream);
}

void Compiler::compileSource(const std::string& source, std::ostream& outputStream) {
    tokens = lexer.lex(source);
    ast = parser.parse(tokens);
    ast->accept(analyzer);
    ast->accept(generator);
    generator.writeBytecode(outputStream);
    ast->accept(this->depGraph);
    auto tempOblock = this->depGraph.getOblockMap();  


    /*
    this->symGraph.setMetadata(this->depGraph.getOblockMap(), analyzer.getOblockDescriptors()); 
    ast->accept(this->symGraph); 
    this->symGraph.annotateControllerDivide(); 

    auto divider = this->symGraph.getDivisionData(); 

    // Verify that the deviders work!
    for(auto& obj : divider.ctlMetaData){
        std::cout<<"Printing data for controller ID: "<<obj.first<<std::endl; 
        for(auto data : obj.second.oblockData){
            std::cout<<"Original oblock: "<<data.first<<std::endl; 
            std::cout<<"Params: "<<std::endl;
            for(auto param : data.second.parameterList){
                std::cout<<param<<" "; 
            }
            std::cout<<"\n"; 
            std::cout<<"Jamar device"<<std::endl; 
            for(auto jamar : data.second.oblockDesc.binded_devices){
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
        for(auto& desc : cntSrc.oblockDesc){
            std::cout<<"\noblockname: "<<desc.first<<std::endl; 
            std::cout<<"Internal name:"<<desc.second.name<<std::endl; 
            for(auto& dev : desc.second.binded_devices){
                std::cout<<"Binded: "<<dev.device_name<<std::endl; 
            }
        }   
    }

    std::cout<<"\nend Split ctl"<<std::endl;
    */ 

    ast->accept(masterInterpreter);
        
    auto& descriptors = analyzer.getOblockDescriptors();

    auto& masterOblocks = masterInterpreter.getOblocks();
    euInterpreters.assign(descriptors.size(), masterInterpreter);
    for (auto&& [descPair, interpreter] : boost::combine(descriptors, euInterpreters)) {
        auto& [oblockName, descriptor] = descPair;
        if (!masterOblocks.contains(oblockName)) { continue; }
        auto& oblock = masterOblocks.at(oblockName);
        oblocks.emplace(oblockName, [&oblock, &interpreter](std::vector<BlsType> v) { return oblock(interpreter, v); });
        oblockDescriptors.push_back(descriptor);
    }

}