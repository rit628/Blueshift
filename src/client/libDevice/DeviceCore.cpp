#include "DeviceCore.hpp"

void DeviceCore::addFileIWatch(std::string &fileName, std::function<bool()> handler){
    if(!hasInterrupt){
        hasInterrupt = true; 
    }
    this->Idesc_list.push_back({SrcType::UNIX_FILE, handler, fileName, 0}); 
}

void DeviceCore::addGPIOIWatch(int gpio_port, std::function<bool(int, int, uint32_t)> interruptHandle){
    if(!hasInterrupt){
        hasInterrupt = true; 
    }
    this->Idesc_list.push_back({SrcType::GPIO, interruptHandle, "", gpio_port}); 
}