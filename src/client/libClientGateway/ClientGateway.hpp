#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "include/Common.hpp"
#include "libScheduler/scheduler.hpp"
#include "libTSQ/TSQ.hpp"
#include "libProcessing/Processing.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"



class ClientEU; 


class ClientEUCache{
    private: 
        // Member Variables
        TriggerManager triggerMan; 
        std::unordered_map<DeviceID, DeviceBox> deviceInfo;
        OblockID oblockName; 

    public: 
        ClientEUCache(OBlockDesc &odesc)
        : triggerMan(odesc)
        {
            this->oblockName = odesc.name; 

            for(DeviceDescriptor& desc : odesc.binded_devices){
                DeviceID devID = desc.device_name; 
                auto& devBox = this->deviceInfo[devID]; 
                devBox.deviceName = desc.device_name;
                devBox.devDropRead = desc.dropRead; 
                devBox.stateQueues = std::make_shared<TSQ<DynamicMasterMessage>>(); 
            }; 
        }

        // EMS state message
        EMStateMessage grabStates(int priority)
        {
            EMStateMessage ems; 
            std::vector<DynamicMasterMessage> dmmList; 

            for(auto& pair : this->deviceInfo){
                auto& devBox = pair.second; 
                if(!devBox.stateQueues->isEmpty()){
                    dmmList.push_back(devBox.stateQueues->read());
                }
                else{
                    dmmList.push_back(devBox.lastMessage.get());
                }
            }

            ems.dmm_list = dmmList; 
            ems.oblockName = this->oblockName; 
            ems.priority = priority; 
            ems.protocol = PROTOCOLS::SENDSTATES;
            
            return ems; 
        }

        int insertState(DynamicMasterMessage &dmm){
            DeviceID devName = dmm.info.device; 
            this->deviceInfo[devName].insertState(dmm); 
            int triggerID = 0; 
            bool onSuccess = this->triggerMan.processDevice(devName, triggerID); 
            // user trigger id to calculate priority
            int trig = 10; 
            if(onSuccess){
                return trig; 
            }
            return -1; 
        }
}; 





// Exec unit processor (with mailbox and EU combined)
class ClientEU{
    private: 
        ClientEUCache dataQueue; 
        TSQ<OwnedSentMessage>& clientTransfer; 

        // FINISH
        void transform(EMStateMessage &ems){
            if(ems.protocol == PROTOCOLS::SENDSTATES){
                auto& dmmList = ems.dmm_list; 
                

                
            }
        }
        
    public: 
        ClientEU(OBlockDesc &odesc, TSQ<OwnedSentMessage> &inMsg)
        : dataQueue(odesc), clientTransfer(inMsg) {}

        // FINISH
        void loadBytecode(std::vector<char> &bytecode){
            
        }

        // FINISH
        bool insertState(DynamicMasterMessage &dmm){
            int trigPr = this->dataQueue.insertState(dmm);
            if(trigPr != -1){
                EMStateMessage emsMsg = this->dataQueue.grabStates(trigPr); 
                this->transform(emsMsg); 
            }
        }
}; 


// Kind of the exeuction manager of the client
class ClientGateway{
    
    private: 
        std::unordered_map<OblockID, ClientEU> execUnits; 
        DeviceScheduler scheduler; 
        TSQ<OwnedSentMessage> &inMsg; 
        int controllerID; 
        

        void convSendMessage(DynamicMasterMessage dmm){
            OwnedSentMessage osm;
            // Fix this later
            osm.connection = nullptr; 
        
            osm.sm.prot = Protocol::BEGIN; 
            osm.sm.ctl_code = dmm.info.controller; 
            osm.sm.oblock_id = dmm.info.oblock;    
        } 


    public: 
        ClientGateway(std::vector<OBlockDesc>& oDescList, TSQ<OwnedSentMessage>& inMsgA, int selfID)
        : scheduler(oDescList, [this](DynamicMasterMessage dmm){}), inMsg(inMsgA)
        {
            this->controllerID = selfID; 
            for(auto& desc: oDescList){
                this->execUnits.emplace(desc, ClientEU(desc, inMsg)); 
            }
        }


}; 