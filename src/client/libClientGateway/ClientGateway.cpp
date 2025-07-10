#include "ClientGateway.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "libtype/bls_types.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>




ClientEM::ClientEM(TSQ<OwnedSentMessage> &readLine,TSQ<OwnedSentMessage> &writeLine)
    :   
    clientReadLine(readLine),
    clientWriteLine(writeLine)   
{

}



void ClientEM::config(std::vector<char> &bytecodeList, std::unordered_map<DeviceID, int> &deviceMap, int ctlCode){
     
    this->vmDivider.loadBytecode(bytecodeList); 
    std::vector<OBlockDesc> descList = this->vmDivider.getOblockDescriptors(); 

    this->clientScheduler = std::make_shared<DeviceScheduler>(descList, [this](HeapMasterMessage hmm){
        OwnedSentMessage osm; 
        osm.sm.header.device_code = this->ident_data.deviceMap[hmm.info.device]; 
    }); 

    this->ctlCode = ctlCode; 
    this->ident_data.deviceMap = deviceMap; 
    for(auto& pair : this->ident_data.deviceMap){
        this->ident_data.intToDev[pair.second] = pair.first; 
    }

    int i = 0; 
    
    for(auto& odesc : descList){
        if(odesc.hostController == "MASTER"){
            continue; 
        }

        std::cout<<"Decentralized: "<<odesc.name<<std::endl; 
        // Populate the in list: 
        for(auto& dev : odesc.binded_devices){
            this->devToOblockMap[dev.device_name].push_back(odesc.name); 
            deviceOutMaps.emplace(dev.device_name , std::make_unique<OutQueue>(this->clientWriteLine)); 
        }

        this->ident_data.oblockMap[odesc.name] = i; 
        this->clientMap.emplace(odesc.name ,std::make_unique<ClientEU>(odesc, this->clientScheduler, this->ident_data, ctlCode, bytecodeList, this->deviceOutMaps));
        i++;  
    }

    this->isConfigured = true; 
    this->beginVMs(); 

}


// Convert the sentMessage to a 
HeapMasterMessage ClientEM::getHMM(SentMessage &toConvert, PROTOCOLS pcol){
    HeapMasterMessage hmm; 
    DynamicMessage dm; 
    dm.Capture(toConvert.body); 
    hmm.heapTree = dm.toTree(); 
    hmm.info.controller = toConvert.header.ctl_code; 
    hmm.info.device = this->ident_data.intToDev[toConvert.header.device_code]; 
    hmm.protocol = pcol; 

    return hmm; 

}

void ClientEM::beginVMs(){
     for(auto& pair : this->clientMap){
        this->euThreads.emplace_back(std::bind(&ClientEU::run, std::ref(*pair.second), std::placeholders::_1)); 
    }
}


// Run the Exec Manager and Units
void ClientEM::run(std::stop_token st){

    while(!st.stop_requested()){
        auto ogMsg = this->clientReadLine.read(); 
        auto message = ogMsg.sm;
        switch(message.header.prot){
            case Protocol::CALLBACK: {
                std::cout<<"DECENTRALIZED CALLBACK RECEIEVED"<<std::endl; 
                auto& devName = this->ident_data.intToDev[message.header.device_code]; 
                this->deviceOutMaps.at(devName)->updateCallback(); 
            }
            case Protocol::SEND_ARGUMENT: {
                std::cout<<"Recieved arguemnt"<<std::endl; 
                HeapMasterMessage hmm = getHMM(message, PROTOCOLS::SENDSTATES);
                auto& transferList = this->devToOblockMap.at(hmm.info.device);
                for(auto& oblock : transferList){
                    this->clientMap[oblock]->insertDevice(hmm); 
                }
                break; 
            }
            case Protocol::OWNER_GRANT : {
                std::cout<<"Received Owner Grant"<<std::endl; 
                HeapMasterMessage hmm = getHMM(message, PROTOCOLS::OWNER_GRANT);
                this->clientScheduler->receive(hmm); 
                break; 
            }
            case Protocol::OWNER_CONFIRM_OK : {
                std::cout<<"Received Confirmation OK"<<std::endl; 
                HeapMasterMessage hmm = getHMM(message, PROTOCOLS::OWNER_CONFIRM_OK);
                this->clientScheduler->receive(hmm);
                break; 
            }
        
            default: {
                // Forward Packet
                std::cout<<"MESSAGE PASS THROUGH CLIENT"<<std::endl; 
                this->clientWriteLine.write(ogMsg); 
                break; 
            }
        }
    }
}



ClientEU::ClientEU(OBlockDesc &odesc, std::shared_ptr<DeviceScheduler> devSche, IdentData &data, int ctlCode, std::vector<char> &bytecode, 
    std::unordered_map<DeviceID, std::unique_ptr<OutQueue>> &outLine)
:EuCache(false, false, odesc.name, odesc), clientScheduler(devSche), idMaps(data), outLines(outLine)
{   
    this->byteCodeSerialized = bytecode; 
    virtualMachine.loadBytecode(bytecode); 
    this->bytecodeOffset = odesc.bytecode_offset; 
    this->name = odesc.name; 
    this->oinfo = odesc; 
    this->clientScheduler = devSche; 


    int i = 0; 
    for(auto& devPos : odesc.binded_devices){
        this->devPosMap[devPos.device_name] = i; 
        i++; 
    }
}; 


void ClientEU::insertDevice(HeapMasterMessage hmm){
    this->EuCache.insertState(hmm, this->reciever); 
}

void ClientEU::replaceCache(std::unordered_map<DeviceID, HeapMasterMessage> &currArgs){
    for(auto& pair:  currArgs){
        if(this->replacementCache.contains(pair.first)){ 
            currArgs[pair.first] = this->replacementCache.get(pair.first).value(); 
        }
    }
}

// converts an hmm into a sent message
SentMessage ClientEU::createSentMessage(BlsType &newData, DeviceDescriptor &devDesc, Protocol pcode){
    SentMessage sm; 
    sm.header.ctl_code = this->ctlCode; 
    // We need to add the conversion factors
    sm.header.device_code = this->idMaps.deviceMap[devDesc.device_name]; 
    sm.header.oblock_id = this->idMaps.oblockMap[this->oinfo.name]; 
    sm.header.prot = pcode; 
    sm.header.body_size = 0;

    DynamicMessage dm; 
    if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(newData)){
        dm.makeFromRoot(std::get<shared_ptr<HeapDescriptor>>(newData)); 
        sm.body = dm.Serialize(); 
        sm.header.body_size = sm.body.size(); 
    }
    
    return sm; 
}


void ClientEU::run(std::stop_token st){
    while(!st.stop_requested()){
        auto obj =this->reciever.read(); 

        // Make HMM map: 
        std::unordered_map<DeviceID, HeapMasterMessage> heapMap; 
        for(auto& hmm : obj.dmm_list){
            heapMap[hmm.info.device] = hmm; 
        }

        // Open and close the packet flow-through valve while waiting for confirm_oks (RN the local )
        this->EuCache.forwardPackets = true; 
        //this->clientScheduler->request(this->name, obj.priority);
        this->EuCache.forwardPackets = false; 

        this->replaceCache(heapMap); 

        std::vector<BlsType> transformStates; 

        for(auto& devDesc : this->oinfo.binded_devices){
            if(heapMap.contains(devDesc.device_name)){
                transformStates.push_back((heapMap.at(devDesc.device_name).heapTree)); 
            }
            else{
                transformStates.push_back(devDesc.initialValue); 
            } 
        }

        // Transformed object:
        this->virtualMachine.setOblockOffset(this->bytecodeOffset);
        auto transformedStates = this->virtualMachine.transform(transformStates); 

        // Transformed the object: 
        for(auto& dev : this->oinfo.outDevices){
            OwnedSentMessage osm; 
            osm.sm = createSentMessage(transformedStates.at(this->devPosMap[dev.device_name]), dev, Protocol::STATE_CHANGE) ; 
            osm.connection = nullptr; 
            this->outLines.at(dev.device_name)->insertMessage(osm); 
        }  

        //this->clientScheduler->release(this->oinfo.name); 
    }
}
