#pragma once 
#include <string> 
#include "include/Common.hpp"
#include "libnetwork/Protocol.hpp"
#include "libTSQ/TSQ.hpp"
#include <unordered_set>

using DevAlias = std::string; 
using ControllerAlias = uint16_t; 
using AttrAlias = std::string; 
using OBlockAlias = std::string; 
using TimerID = uint16_t; 


// Within a device, maps polling rate to list of associated oblocks
using TimerList = std::unordered_map<int, TimerID>; 
using AttrVol = std::unordered_map<AttrAlias, float>; 

struct Conditional{
    // RHS and LHS sign
    float rhs_arg; 
    float lhs_arg; 
    // Device associated with the object
    DevAlias device; 
    // Field associated with the string
    AttrAlias field; 
}; 


// Used to transfer a volatility; 
struct Volatility{
    // Device name
    DevAlias device_name; 
    // attribute alias
    AttrAlias attr_alias; 
    // Standard deviation data used to measure the volatility
    float std; 
}; 

struct TimerDesc{
    TimerID id;
    std::vector<OBlockAlias> oblocks; 
    int period; 
    bool isConst; 
}; 


// Permits a device to maintain several Constant Timer Objects and one dynamic timer; 
struct TimerInfo{
    std::vector<TimerID> const_timers; 
    TimerID dynamic_time = 0; 
}; 


// Master Ticker Table; 
class MTicker{
    private:    
        // Maps devices + polling rates to associated oblocks; 
        std::unordered_map<DevAlias, TimerInfo> ticker_table; 
        std::unordered_map<TimerID, TimerDesc> timer_map; 

        
        // Map of volatilities: 
        std::unordered_map<DevAlias, AttrVol> volMap; 

        // Reference to an device alias map: 
        std::unordered_map<std::string, std::unordered_set<DevAlias>> ctl_device_map; 

        std::unordered_map<DevAlias, DeviceDescriptor> device_data; 

    public: 
        // Intialize the tocker table: 
        MTicker(std::vector<OBlockDesc> &OBlocks);


        // Thread updates volatility from the vol TSQ
        void updateVol(); 
        // Updates single volatility object: 
        void updateVolH(DevAlias& devName, std::unordered_map<AttrAlias, float>& dataMap); 

        // Thread reads the volati
        void updateSA(); 
        // Updates single ticker entry from SA line
        void updateSAH(std::vector<Conditional> &conditional_data); 


        // Send the inital message with constants;  
        void sendInitial(std::vector<Timer> &timerVector, std::string &ctl, std::unordered_map<std::string, uint16_t> &device_map); 
        // send the remaining messages (only dynamic rates)
        void sendTicker(std::vector<Timer> &timerVector, std::string &ctl, std::unordered_map<std::string, uint16_t> &device_map); 

        // Get Oblocks list: given a timerID get the list of associated oblocks
        std::vector<OBlockAlias>& getOblocks(TimerID id); 

        

}; 