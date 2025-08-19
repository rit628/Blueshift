#ifdef __RPI64__
#include "include/DHT11.hpp"
#include "libDM/DynamicMessage.hpp"

using namespace Device;

void DHT11::init(std::unordered_map<std::string, std::string> &config){
    this->PIN = std::stoi(config.at("PIN"));
} 

void DHT11::processStates(DynamicMessage &dmsg){
    // Read only
}

void DHT11::transmitStates(DynamicMessage &dmsg){
    int chk = this->dht.readDHT11(this->PIN);
    int numTries = 15;
    for(int i = 0 ; i < numTries ; i++){
        if(chk == DHTLIB_OK){
            break;
        }
    }

    this->states.humidity = this->dht.humidity;
    this->states.temperature = this->dht.temperature;
    dmsg.packStates(this->states);
}



#endif