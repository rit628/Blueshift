#ifdef __linux__

#include "include/READ_FILE_POLL.hpp"

void Device<TypeDef::READ_FILE_POLL>::set_ports(std::unordered_map<std::string, std::string> &srcs) {
    this->filename = "./samples/client/" + srcs["file"];
    this->file_stream.open(filename);
    if(this->file_stream.is_open()) {
        std::cout<<"Could find file"<<std::endl;
    } 
    else{
        std::cout<<"Could not find file"<<std::endl;
    }
}

void Device<TypeDef::READ_FILE_POLL>::proc_message(DynamicMessage &dmsg) {

}

void Device<TypeDef::READ_FILE_POLL>::read_data(DynamicMessage &dmsg) {
    std::getline(this->file_stream, states.msg);
    this->file_stream.seekg(0, std::ios::beg);
    dmsg.packStates(states);
}

Device<TypeDef::READ_FILE_POLL>::~Device() {
    this->file_stream.close();
}

#endif