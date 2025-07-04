#include "ClientRouter.hpp"
#include "include/Common.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"


ClientRouter::ClientRouter(TSQ<SentMessage> &loopback)
: loopbackQueue(loopback)
{
     
}


void ClientRouter::setConnection(ControllerID& target, std::shared_ptr<Connection> controller){
    this->connectionMap[target] = controller; 
}

void ClientRouter::createRoute(uint16_t device, std::vector<ControllerID>& contID){
    this->pathing[device] = contID; 
}

