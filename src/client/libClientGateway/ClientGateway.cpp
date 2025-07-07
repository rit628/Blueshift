#include "ClientGateway.hpp"
#include "include/Common.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libnetwork/Protocol.hpp"
#include "libtype/bls_types.hpp"
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>




ClientEM::ClientEM(std::vector<OBlockDesc> &descList, std::vector<std::vector<char>> &bytecodeList, TSQ<SentMessage> &readLine,
    TSQ<SentMessage> &writeLine,std::unordered_map<DeviceID, int> data, int ctlCode)
:   clientScheduler(descList, [this](HeapMasterMessage hmm){}), 
    clientReadLine(readLine),
    clientWriteLine(writeLine)
    
{
    int i = 0; 
    for(auto& odesc : descList){
        // Populate the in list: 
        for(auto& dev : odesc.inDevices){
            this->devToOblockMap[dev.device_name].push_back(odesc.name); 
        }

        this->ident_data.oblockMap[odesc.name] = i; 
        this->clientMap.emplace(odesc.name ,std::make_unique<ClientEU>(odesc, this->clientScheduler, writeLine, this->ident_data, ctlCode, bytecodeList[i]));
        i++;  
    }

    this->ctlCode = ctlCode; 
    this->ident_data.deviceMap = data; 
    for(auto& pair : this->ident_data.deviceMap){
        this->ident_data.intToDev[pair.second] = pair.first; 
    }
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


// Run the Exec Manager and Units
void ClientEM::run(){
    while(true){
        auto message = this->clientReadLine.read();
        switch(message.header.prot){
            case Protocol::SEND_STATE:{
                // forward packet 
                this->clientWriteLine.write(message); 
                break; 
            }
            case Protocol::SEND_ARGUMENT: {
                HeapMasterMessage hmm = getHMM(message, PROTOCOLS::SENDSTATES);
                auto& transferList = this->devToOblockMap[hmm.info.device];
                for(auto& oblock : transferList){
                    this->clientMap[oblock]->insertDevice(hmm); 
                }
                break; 
            }
            case Protocol::OWNER_GRANT : {
                HeapMasterMessage hmm = getHMM(message, PROTOCOLS::OWNER_GRANT);
                this->clientScheduler.receive(hmm); 
                break; 
            }
            case Protocol::OWNER_CONFIRM_OK : {
                HeapMasterMessage hmm = getHMM(message, PROTOCOLS::OWNER_CONFIRM_OK);
                this->clientScheduler.receive(hmm);
                break; 
            }
            default: {
                // Forward Packet
                this->clientWriteLine.write(message); 
                break; 
            }
        }
    }
}


ClientEU::ClientEU(OBlockDesc &odesc, DeviceScheduler &devSche, TSQ<SentMessage> &mainLine, IdentData &data, int ctlCode, std::vector<char> &bytecode)
:EuCache(false, false, odesc.name, odesc), scheObj(devSche), clientMainLine(mainLine),
idMaps(data)
{   
    this->byteCodeSerialized = bytecode; 
    virtualMachine.loadBytecode("REPLACE WITH VECTOR"); 
    this->bytecodeOffset = odesc.bytecode_offset; 
    this->name = odesc.name; 
    this->oinfo = odesc; 

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


void ClientEU::run(){
    while(true){
        auto obj =this->reciever.read(); 
        
        // Make HMM map: 
        std::unordered_map<DeviceID, HeapMasterMessage> heapMap; 
        for(auto& hmm : obj.dmm_list){
            heapMap[hmm.info.device] = hmm; 
        }

        // Open and close the packet flow-through valve while waiting for confirm_oks
        this->EuCache.forwardPackets = true; 
        this->scheObj.request(this->name, obj.priority);
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
        this->virtualMachine.transform(this->bytecodeOffset, transformStates); 

        // Transformed the object: 
        for(auto& dev : this->oinfo.outDevices){
            this->clientMainLine.write(
                createSentMessage(transformStates.at(this->devPosMap[dev.device_name]), dev, Protocol::SEND_STATE)  
            ); 
        }  

        this->scheObj.release(this->oinfo.name); 
    }
}
