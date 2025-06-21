#ifdef __linux__

#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "include/TIMER_TEST.hpp"

using namespace Device;

void TIMER_TEST::processStates(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);
    write_file << std::to_string(this->states.test_val) << ",";  
    write_file.flush(); 
}

void TIMER_TEST::init(std::unordered_map<std::string, std::string> &config) {
    filename = "./samples/client/" + config["file"]; 
    write_file.open(filename);
    if(write_file.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl;
        throw BlsExceptionClass("You suck!", ERROR_T::BAD_DEV_CONFIG); 
    }
}

void TIMER_TEST::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

TIMER_TEST::~TIMER_TEST() {
    write_file.close();
}

#endif