#pragma once

#include "DeviceCore.hpp"
#include <iostream>
#include <fstream>

/*
    This port just sends out the info data 10 over and over again. It is a 
*/

class TestTimer : public AbstractDevice{
    private: 
        int val = 0; 
        std::string filename; 
        std::ofstream write_file; 

    public: 

        void set_ports(std::unordered_map<std::string, std::string> &srcs) override {
            filename = "./testDir/" + srcs["file_name"]; 
            write_file.open(filename);
            if(write_file.is_open()){
                std::cout<<"Could find file"<<std::endl; 
            } 
            else{
                std::cout<<"Could not find file"<<std::endl; 
            }
        }

        // Process and write message to file
        void proc_message(DynamicMessage dmsg) override{
            int omar; 
            std::cout<<"HEHEHEHEHEHE"<<std::endl; 
            if(dmsg.hasField("test_val")){
                dmsg.unpack("test_val", omar);
                this->val = omar; 
                write_file << std::to_string(this->val) << ",";  
                write_file.flush(); 
            }
            else{
                std::cout<<"huh?"<<std::endl; 
            }
        }


        void read_data(DynamicMessage &dmsg) override {
            dmsg.createField("test_val", this->val); 
        }
}; 

/*
    This Interrupt type device reads messages from a file (consdiers the first line)
    and sends the contents of the first line of the file once the return button is pressed

*/


class StringReader : public AbstractDevice{
    private: 
        std::string filename; 
        std::ifstream file_stream; 
        std::string curr_message; 

    public: 
        // can add better keybinding libraries in the future
        bool handleInterrupt(){
            this->file_stream.clear(); 
            this->file_stream.seekg(0, std::ios::beg); 

            std::string line; 
            if(std::getline(this->file_stream, line)){
                this->curr_message = line; 
                return true; 
            }
          
            return false; 
        }
        
        void set_ports(std::unordered_map<std::string, std::string> &src) override {
            filename = "./testDir/" + src["file"]; 
            file_stream.open(filename, std::ios::in); 
            if(file_stream.is_open()){
                std::cout<<"Could find file"<<std::endl; 
            }
            else{
                std::cout<<"Could not find file"<<std::endl; 
            }

            // Add the interrupt and handler
            this->addFileIWatch(this->filename, [this](){return this->handleInterrupt();}); 
        }

        void read_data(DynamicMessage &dmsg) override {
            dmsg.createField("msg", this->curr_message); 
        }
}; 



// Device reciever object returns and sets up an object 
inline std::shared_ptr<AbstractDevice> getDevice(DEVTYPE dtype, std::unordered_map<std::string, std::string> &port_nums, int device_alias){
    switch(dtype){
        // Used to test the timer functionality
        case(DEVTYPE::TIMER_TEST) : {
            auto devPtr = std::make_shared<TestTimer>(); 
            devPtr->set_ports(port_nums); 
            return devPtr;
            break; 
        }
        case(DEVTYPE::LINE_WRITER) : {
            auto devPtr = std::make_shared<StringReader>(); 
            devPtr->set_ports(port_nums); 
            return devPtr; 
            break; 
        }
        default : {
            throw std::invalid_argument("Unknown dtype accessed!"); 
        }
    }; 
}; 
