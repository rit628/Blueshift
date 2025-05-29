#ifdef __linux__

#include "include/TIMER_TEST.hpp"

void Device<TypeDef::TIMER_TEST>::processStates(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);
    write_file << std::to_string(this->states.test_val) << ",";  
    write_file.flush(); 
}

void Device<TypeDef::TIMER_TEST>::init(std::unordered_map<std::string, std::string> &config) {
    filename = "./samples/client/" + config["file"]; 
    write_file.open(filename);
    if(write_file.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }
}

void Device<TypeDef::TIMER_TEST>::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

Device<TypeDef::TIMER_TEST>::~Device() {
    write_file.close();
}

#endif