#include "libtypes/typedefs.hpp"
#ifdef __linux__

#include "include/FILE_LOG.hpp"

void Device<TypeDef::FILE_LOG>::processStates(DynamicMessage& dmsg) {
    dmsg.unpackStates(this->states);
    std::cout<<"Processing message: "<<states.add_msg<<std::endl;
    this->outStream<<this->states.add_msg<<std::endl;
}

void Device<TypeDef::FILE_LOG>::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"]; 
    this->outStream.open(filename);
    if(this->outStream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }

    this->addFileIWatch(filename, []{return true;}); 
}

void Device<TypeDef::FILE_LOG>::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(this->states);
}

Device<TypeDef::FILE_LOG>::~Device() {
    this->outStream.close();
}

#endif