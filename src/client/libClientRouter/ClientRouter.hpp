#include "include/Common.hpp"
# include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include <unordered_map>

using cont_int = uint16_t; 
using dev_int = uint16_t; 
using oblock_int = uint16_t; 


// Routes objects to send them
class ClientRouter{
    private: 
        std::unordered_map<ControllerID, std::shared_ptr<Connection>> connectionMap; 
        TSQ<SentMessage> &loopbackQueue; 
        std::unordered_map<uint16_t, std::vector<ControllerID>> pathing; 
    public: 
        ClientRouter(TSQ<SentMessage> &loopBack); 
        void setConnection(ControllerID& targCont, std::shared_ptr<Connection> conObj);  
        void createRoute(uint16_t device, std::vector<ControllerID> &ctlGroup); 
        void sendStates(uint16_t deviceCode, Protocol type, bool fromInt = false, oblock_int oint = 0);
}; 