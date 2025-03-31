#pragma once

#include "DeviceCore.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libtypes/typedefs.hpp"
#include <atomic>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sys/fcntl.h>
#include <chrono>
#include <thread>

/*
    This port just sends out the info data 10 over and over again. It is a 
*/

class TestTimer : public AbstractDevice{
    private: 
        TypeDef::TIMER_TEST states;
        std::string filename; 
        std::ofstream write_file; 

        // Process and write message to file
        void proc_message_impl(DynamicMessage& dmsg) override {
            dmsg.unpackStates(states);
            write_file << std::to_string(this->states.test_val) << ",";  
            write_file.flush(); 
        }

    public: 

        void set_ports(std::unordered_map<std::string, std::string> &srcs) override {
            filename = "./samples/client/" + srcs["file"]; 
            write_file.open(filename);
            if(write_file.is_open()){
                std::cout<<"Could find file"<<std::endl; 
            } 
            else{
                std::cout<<"Could not find file"<<std::endl; 
            }
        }

        void read_data(DynamicMessage &dmsg) override {
            dmsg.packStates(states);
        }
}; 

/*
    This Interrupt type device reads messages from a file (consdiers the first line)
    and sends the contents of the first line of the file once the return button is pressed

*/


class StringReader : public AbstractDevice{
    private:
        TypeDef::LINE_WRITER states;
        std::string filename; 
        std::fstream file_stream; 

        void proc_message_impl(DynamicMessage& dmsg) override {
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
        
    public: 
        // can add better keybinding libraries in the future
        bool handleInterrupt(){

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
        
        void set_ports(std::unordered_map<std::string, std::string> &src) override {
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

        void read_data(DynamicMessage &dmsg) override {
            dmsg.packStates(states);
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
