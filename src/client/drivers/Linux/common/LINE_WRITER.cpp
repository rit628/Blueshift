#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#ifdef __linux__

#include "include/LINE_WRITER.hpp"

void Device<TypeDef::LINE_WRITER>::processStates(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);

    if (this->file_stream.is_open()) {
        this->file_stream.close();  // Close the file before reopening
    }
    
    this->file_stream.open(this->filename, std::ios::out | std::ios::trunc);  // Open in truncate mode
    
    if (this->file_stream.is_open()) {
        std::cout<<"Writing data"<<std::endl; 
        this->file_stream << states.msg; 
        this->file_stream.flush();  // Ensure data is written immediately
        this->file_stream.close();
    }
    else {
        std::cout << "file didnt open" << std::endl;
    }
}

// can add better keybinding libraries in the future
bool Device<TypeDef::LINE_WRITER>::handleInterrupt(){

    this->file_stream.open(this->filename, std::ios::in);
    this->file_stream.clear(); 
    this->file_stream.seekg(0, std::ios::beg); 

    std::string line; 

    auto getL = bool(std::getline(this->file_stream, line));

    if(getL){
        this->states.msg = line;
        return true; 
    }
    else {
        std::cout << "unc status" << std::endl;
    }
    
    return false; 
}

void Device<TypeDef::LINE_WRITER>::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"]; 
    
    this->file_stream.open(filename, std::ios::in | std::ios::out); 
    if(file_stream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    }
    else{
        std::cout<<"Could not find file"<<std::endl; 
        throw BlsExceptionClass("LINE WRITER " + this->filename, ERROR_T::BAD_DEV_CONFIG);
    }

    // Add the interrupt and handler
    this->addFileIWatch(this->filename, [this](){return this->handleInterrupt();}); 
}

void Device<TypeDef::LINE_WRITER>::transmitStates(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

Device<TypeDef::LINE_WRITER>::~Device() {
    file_stream.close();
}

#endif