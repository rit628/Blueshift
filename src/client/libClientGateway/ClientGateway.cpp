#include "ClientGateway.hpp"
#include "Serialization.hpp"
#include "DynamicMessage.hpp"
#include "Scheduler.hpp"
#include "Protocol.hpp"
#include "bls_types.hpp"
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>


ClientEM::ClientEM(std::vector<TaskDescriptor> &descList, TSQ<SentMessage> &readLine,
    TSQ<SentMessage> &writeLine,std::unordered_map<DeviceID, int> data, int ctlCode)
:   clientScheduler(descList, [](HeapMasterMessage){}), 
    clientReadLine(readLine),
    clientWriteLine(writeLine)
{
    int i = 0; 
    for(auto& taskDesc : descList){
        // Populate the in list: 
        for(auto& dev : taskDesc.inDevices){
            this->devToTaskMap[dev.device_name].push_back(taskDesc.name); 
        }

        this->ident_data.taskMap[taskDesc.name] = i; 
        this->clientMap.emplace(taskDesc.name ,std::make_unique<ClientEU>(taskDesc, this->clientScheduler, writeLine, this->ident_data));
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
                auto& transferList = this->devToTaskMap[hmm.info.device];
                for(auto& task : transferList){
                    this->clientMap[task]->insertDevice(hmm); 
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


ClientEU::ClientEU(TaskDescriptor &taskDesc, DeviceScheduler &devSche, TSQ<SentMessage> &mainLine, IdentData &data)
:EuCache(false, false, taskDesc.name, taskDesc), scheObj(devSche), clientMainLine(mainLine),
idMaps(data)
{   
    virtualMachine.loadBytecode("PLACEHOLDER");
    this->bytecodeOffset = taskDesc.bytecode_offset; 
    this->name = taskDesc.name; 
    this->taskInfo = taskDesc; 

    int i = 0; 
    for(auto& devPos : taskDesc.binded_devices){
        this->devPosMap[devPos.device_name] = i; 
        i++; 
    }
}


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
    sm.header.task_id = this->idMaps.taskMap[this->taskInfo.name]; 
    sm.header.prot = pcode; 
    sm.header.body_size = 0;

    DynamicMessage dm; 
    if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(newData)){
        dm.makeFromRoot(std::get<std::shared_ptr<HeapDescriptor>>(newData)); 
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

        for(auto& devDesc : this->taskInfo.binded_devices){
            if(heapMap.contains(devDesc.device_name)){
                transformStates.push_back((heapMap.at(devDesc.device_name).heapTree)); 
            }
            else{
                transformStates.push_back(devDesc.initialValue); 
            } 
        }

        // Transformed object:
        this->virtualMachine.setTaskOffset(this->bytecodeOffset);
        auto transformedStates = this->virtualMachine.transform(transformStates);

        // Transformed the object: 
        for(auto& dev : this->taskInfo.outDevices){
            this->clientMainLine.write(
                createSentMessage(transformStates.at(this->devPosMap[dev.device_name]), dev, Protocol::SEND_STATE)  
            ); 
        }  

        this->scheObj.release(this->taskInfo.name); 
    }
}
