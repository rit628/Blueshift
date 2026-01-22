#include "include/HTTP_ENDPOINT.hpp"
#include "DynamicMessage.hpp"
#include <unordered_map>

using namespace Device; 


void HTTP_ENDPOINT::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(states); 
    this->listener->write(states.sessionID, states.responseJson , states.responseCode);
}

bool HTTP_ENDPOINT::handleRequest(int sessionID, std::string ip, std::string json){
    states.requestJson = json; 
    states.requestMethod = ip;  
    states.sessionID = sessionID; 
    return true; 
}

void HTTP_ENDPOINT::init(std::unordered_map<std::string, std::string> &config){
    std::string endpoint = config.at("endpoint"); 
    auto reqBound = std::bind(&HTTP_ENDPOINT::handleRequest, std::ref(*this), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3); 
    this->listener = this->addEndpointIWatch(endpoint,  reqBound); 
}


void HTTP_ENDPOINT::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(states); 
}

