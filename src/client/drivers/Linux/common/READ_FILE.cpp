#include "libtypes/typedefs.hpp"
#ifdef __linux__

#include "include/READ_FILE.hpp"

void Device<TypeDef::READ_FILE>::processStates(DynamicMessage& dmsg) {

}

void Device<TypeDef::READ_FILE>::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"];
    this->file_stream.open(filename);
    if(this->file_stream.is_open()){
        std::cout<<"Could find file"<<std::endl;
    }
    else{
        std::cout<<"Could not find file"<<std::endl;
    }
    this->addFileIWatch(this->filename);
}

void Device<TypeDef::READ_FILE>::transmitStates(DynamicMessage &dmsg) {
    this->file_stream.seekg(0, std::ios::beg);
    std::getline(this->file_stream, states.msg); 
    dmsg.packStates(states); 
}

Device<TypeDef::READ_FILE>::~Device() {
     this->file_stream.close();
}

#endif