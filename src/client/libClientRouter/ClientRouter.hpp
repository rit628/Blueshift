#include "include/Common.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include <unordered_map>
#include <vector> 
#include <string> 

using OblockID = std::string; 
using DeviceID = std::string; 
using ControllerID = std::string; 

struct RoutingInfo{
    DeviceID originDev;
    std::vector<shared_ptr<Connection>> connectionSources; 
    bool routeToSelf = false; 
}; 


class Router{
    private:
        // Device Routing Map
        std::unordered_map<DeviceID, RoutingInfo> routingMap; 
        std::shared_ptr<Connection>& masterConnection; 
    public: 
        Router(std::shared_ptr<Connection> &masterCon)
        : masterConnection(masterCon){}
        void setRoutes(SentMessage &dmsg);

}; 