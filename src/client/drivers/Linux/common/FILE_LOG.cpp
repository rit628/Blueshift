#ifdef __linux__

#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "libtypes/typedefs.hpp"
#include "include/FILE_LOG.hpp"

using namespace Device;

void FILE_LOG::processStates(DynamicMessage& dmsg) {
    dmsg.unpackStates(this->states);
    std::cout<<"Processing message: "<<states.add_msg<<std::endl;
    this->outStream<<this->states.add_msg<<std::endl;
}

void FILE_LOG::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"]; 
    this->outStream.open(filename);
    if(this->outStream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
        throw BlsExceptionClass("FILE_LOG: " + this->filename, ERROR_T::BAD_DEV_CONFIG); 
    }

    this->addFileIWatch(filename, []{return true;}); 
}

void FILE_LOG::transmitStates(DynamicMessage &dmsg) {
    std::cout<<"State add_msg value: "<<this->states.add_msg<<std::endl;
    dmsg.packStates(this->states);
}

FILE_LOG::~FILE_LOG() {
    this->outStream.close();
}

#endif