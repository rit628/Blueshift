#ifdef __linux__

#include "../../include/TIMER_TEST.hpp"

void Device<TypeDef::TIMER_TEST>::proc_message(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);
    write_file << std::to_string(this->states.test_val) << ",";  
    write_file.flush(); 
}

void Device<TypeDef::TIMER_TEST>::set_ports(std::unordered_map<std::string, std::string> &srcs) {
    filename = "./samples/client/" + srcs["file"]; 
    write_file.open(filename);
    if(write_file.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }
}

void Device<TypeDef::TIMER_TEST>::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

Device<TypeDef::TIMER_TEST>::~Device() {
    write_file.close();
}

#endif