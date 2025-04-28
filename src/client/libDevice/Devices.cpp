#include "Devices.hpp"

using namespace Device;

/* TIMER_TEST */

void TIMER_TEST::proc_message_impl(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);
    write_file << std::to_string(this->states.test_val) << ",";  
    write_file.flush(); 
}

void TIMER_TEST::set_ports(std::unordered_map<std::string, std::string> &srcs) {
    filename = "./samples/client/" + srcs["file"]; 
    write_file.open(filename);
    if(write_file.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }
}

void TIMER_TEST::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

/* LINE_WRITER */

void LINE_WRITER::proc_message_impl(DynamicMessage& dmsg) {
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
bool LINE_WRITER::handleInterrupt(){

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

void LINE_WRITER::set_ports(std::unordered_map<std::string, std::string> &src) {
    this->filename = "./samples/client/" + src["file"]; 
    
    this->file_stream.open(filename, std::ios::in | std::ios::out); 
    if(file_stream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    }
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }

    // Add the interrupt and handler
    this->addFileIWatch(this->filename, [this](){return this->handleInterrupt();}); 
}

void LINE_WRITER::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

LINE_WRITER::~LINE_WRITER() {
    file_stream.close();
}

/* LINE_WRITER_POLL */
void READ_FILE::set_ports(std::unordered_map<std::string, std::string> &srcs){
    this->filename = "./samples/client/" + srcs["file"]; 
    this->file_stream.open(filename);
    if(this->file_stream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }
    this->addFileIWatch(this->filename); 
}

void READ_FILE::proc_message_impl(DynamicMessage &dmsg){
  
}

void READ_FILE::read_data(DynamicMessage &dmsg){
    this->file_stream.seekg(0, std::ios::beg);
    std::getline(this->file_stream, states.msg); 
    dmsg.packStates(states); 
}

READ_FILE::~READ_FILE(){
    this->file_stream.close(); 
}

/*
    FILE LOG
*/

void FILE_LOG::set_ports(std::unordered_map<std::string, std::string> &srcs){
    this->filename = "./samples/client/" + srcs["file"]; 
    this->outStream.open(filename);
    if(this->outStream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }

    this->addFileIWatch(filename, []{return true;}); 
}


void FILE_LOG::proc_message_impl(DynamicMessage &dmsg){
    dmsg.unpackStates(this->states); 
    std::cout<<"Processing message: "<<states.add_msg<<std::endl; 
    this->outStream<<this->states.add_msg<<std::endl;; 
}

void FILE_LOG::read_data(DynamicMessage &dmsg){
    dmsg.packStates(this->states); 
}

FILE_LOG::~FILE_LOG(){
    this->outStream.close(); 
}



/*
    Get Device Functions:
*/

std::shared_ptr<AbstractDevice> getDevice(DEVTYPE dtype, std::unordered_map<std::string, std::string> &port_nums, int device_alias, uint16_t sendInterrupt) {
    switch(dtype){
        #define DEVTYPE_BEGIN(name) \
        case DEVTYPE::name: { \
            auto devPtr = std::make_shared<name>(); \
            devPtr->set_ports(port_nums); \
            devPtr->isTrigger = sendInterrupt; \
            return devPtr; \ 
            break; \
        }
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        default : {
            throw std::invalid_argument("Unknown dtype accessed!"); 
        }
    }; 
}; 