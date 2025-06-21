#ifdef __linux__

#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "libtypes/typedefs.hpp"
#include "include/READ_FILE.hpp"

using namespace Device;

void READ_FILE::processStates(DynamicMessage& dmsg) {

}

void READ_FILE::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"];
    this->file_stream.open(filename);
    if(this->file_stream.is_open()){
        std::cout<<"Could find file"<<std::endl;
    }
    else{
        std::cout<<"Could not find file"<<std::endl;
        throw BlsExceptionClass("I remember you: " + this->filename, ERROR_T::BAD_DEV_CONFIG); 
    }
    this->addFileIWatch(this->filename);
}

void READ_FILE::transmitStates(DynamicMessage &dmsg) {
    this->file_stream.seekg(0, std::ios::beg);
    std::getline(this->file_stream, states.msg); 
    dmsg.packStates(states); 
}

READ_FILE::~READ_FILE() {
    this->file_stream.close();
}

#endif