#ifdef __linux__

#include "READ_FILE_POLL.hpp"

using namespace Device;

void READ_FILE_POLL::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"];
    this->file_stream.open(filename);
    if(this->file_stream.is_open()) {
        std::cout<<"Could find file"<<std::endl;
    } 
    else{
        std::cout<<"Could not find file"<<std::endl;
    }
}

void READ_FILE_POLL::processStates(DynamicMessage&) {

}

void READ_FILE_POLL::transmitStates(DynamicMessage &dmsg) {
    std::getline(this->file_stream, states.msg);
    this->file_stream.seekg(0, std::ios::beg);
    dmsg.packStates(states);
}

READ_FILE_POLL::~READ_FILE_POLL() {
    this->file_stream.close();
}

#endif