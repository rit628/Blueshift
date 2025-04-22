#include "Ticker.hpp"
#include <boost/math/distributions/normal.hpp>
#include <functional> 


// Maximum polling rate per second (10 ms period or 100 updates per)
#define MAX_POLLR 100

// Thanks Chat gpt
struct PairHash {
    size_t operator()(const std::pair<std::string, int>& p) const {
        size_t h1 = std::hash<std::string>{}(p.first);
        size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 << 1); // Bitwise combination
    }
};

MTicker::MTicker(std::vector<OBlockDesc> &OBlocks)
{
    std::unordered_map<std::pair<std::string, int>, TimerID, PairHash> used_id;
    int curr_id = 1; 

    for(auto &ob : OBlocks){
        for(auto &dev : ob.binded_devices){
            if(dev.isInterrupt){
                continue; 
            }

           DevAlias dev_alias = dev.device_name; 
           this->ctl_device_map[dev.controller].insert(dev_alias); 
           // Device Info; 
           TimerInfo device_info; 
           if(this->ticker_table.find(dev_alias) != this->ticker_table.end()){
                device_info = this->ticker_table[dev_alias]; 
           }
        
           if(dev.isConst){
                auto data_pair = std::make_pair(dev_alias, dev.polling_period); 
                if(used_id.find(data_pair) != used_id.end()){
                    TimerID obj = used_id[data_pair]; 
                    this->timer_map[obj].oblocks.push_back(ob.name); 
                }
                else{

                    TimerDesc newTimer; 
                    newTimer.id = curr_id; 
                    newTimer.oblocks.push_back(ob.name); 
                    newTimer.period =  dev.polling_period; 
                    newTimer.isConst = true; 

                    // Add timer id -> object mapping in the timer
                    this->timer_map[curr_id] = newTimer;

                    // Add ID to constant timer
                    device_info.const_timers.push_back(newTimer.id); 

                    // This is now a used ID, cache it: 
                    used_id[data_pair] = curr_id; 
                }
           }
           else{
                // Make a new TimerDesc if the Dynamic Timer id is unitialized
                if(device_info.dynamic_time == 0){
                    // Establish the id of the dynamic timer
                    device_info.dynamic_time = curr_id; 

                    TimerDesc newTimer; 
                    newTimer.id = curr_id; 
                    // yes, all dynamic prs are initialized to a polling rate of 0.5 secons
                    newTimer.period = 500;
                    newTimer.oblocks.push_back(ob.name); 
                    newTimer.isConst = false; 

                    // add the new timer to the timer id map;
                    this->timer_map[curr_id] = newTimer; 
                } 
                else{
                    int dyn_id = device_info.dynamic_time; 
                    this->timer_map[dyn_id].oblocks.push_back(ob.name); 

                }
           }
           curr_id++; 
           this->ticker_table[dev_alias] = device_info;  
        }
    }
}

Timer makeTimer(TimerDesc tdesc, uint16_t dname){
    Timer newTimer; 
    newTimer.const_poll = tdesc.isConst; 
    newTimer.device_num = dname; 
    newTimer.id = tdesc.id; 
    newTimer.period = tdesc.period; 

    return newTimer; 

}

// Send the initial Message (including the constant timers for a certain ctl);
void MTicker::sendInitial(std::vector<Timer> &timerList, std::string &ctl, std::unordered_map<std::string, uint16_t> &devNames){
    std::unordered_set<DevAlias> omar = this->ctl_device_map[ctl]; 

    for(auto& devName : omar){
        DevAlias dname = devName; 
        TimerInfo tInfo = this->ticker_table[devName]; 


        // Loop through constant timers first; 
        for(auto &tid:  tInfo.const_timers){
            TimerDesc tdesc = this->timer_map[tid]; 
            timerList.push_back(makeTimer(tdesc, devNames[dname])); 

        }
        
        // Get Dynamic Timer
        if(tInfo.dynamic_time != 0){
            TimerID dynId =  tInfo.dynamic_time; 
            TimerDesc newDynTimer = this->timer_map[dynId];
            timerList.push_back(makeTimer(newDynTimer, devNames[dname])); 
        }
    }   
}


void MTicker::sendTicker(std::vector<Timer> &timerList, std::string &ctl, std::unordered_map<std::string, uint16_t> &devNames){
    std::unordered_set<DevAlias> omar = this->ctl_device_map[ctl];  

    // Only receive a subset of devices that belong to controller ctl: 
    for(auto &dev : omar){
        DevAlias dname = dev; 
        TimerInfo tinfo = this->ticker_table[dev]; 
        
        // Only send the dynamic time: 
        TimerID id = tinfo.dynamic_time; 
        if(id != 0){
            timerList.push_back(makeTimer(this->timer_map[id], devNames[dname])); 
        }
    }
}

// Update vol Thread
void MTicker::updateVol(){
    std::cout<<"To be implemented"<<std::endl; 
}

// Volatility object
void MTicker::updateVolH(DevAlias& devName, std::unordered_map<AttrAlias, float>& dataMap){
    // Update the volatility values: (Perhaps add a mutex)
    this->volMap[devName] = dataMap; 
}


// Update vol Thread
void MTicker::updateSA(){
    std::cout<<"To be implemented"<<std::endl; 
}

/* 

Receives the static analysis information from the Exeuction Manager (via the line)
and updates the polling rate using a gaussian function

*/ 

void MTicker::updateSAH(std::vector<Conditional> &conditional){
    for(auto &cond : conditional){
        // Get the volatility for the attribute
        float std = this->volMap[cond.device][cond.field]; 
        float mean = cond.rhs_arg; 
        float tester = cond.lhs_arg; 

        float max_pollr = MAX_POLLR; 

        // Boost normal: 
        boost::math::normal dist(mean, std); 
        auto ans = static_cast<float>(boost::math::cdf(dist, tester)); 
        float scale = 0; 

        if(tester <= mean){
            scale = ans; 
        }
        else if(tester > mean){
            scale = 1 - ans; 
        }

        // Conversion of polling rate (hz) to period (ms)
        float period = 1000/((scale * 0.5) * max_pollr); 

        // Check the current entry in the ticker table: 
        TimerID currTimer = this->ticker_table[cond.device].dynamic_time; 

        // Alter the current time to account for increased urgency
        if(period < this->timer_map[currTimer].period){
            this->timer_map[currTimer].period = static_cast<int>(period); 
        }
    }
}

// maps names to oblocks
std::vector<OBlockAlias>& MTicker::getOblocks(TimerID t_id){
    return this->timer_map[t_id].oblocks; 
}