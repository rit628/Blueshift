#include <iostream>
#include <memory>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <vector>
#include "include/Common.hpp"
#include "libScheduler/Scheduler.hpp"
#include "libTSQ/TSQ.hpp"
#include "libProcessing/Processing.hpp"
#include "libnetwork/Protocol.hpp"
#include "../lang/libvirtual_machine/virtual_machine.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "libtype/bls_types.hpp"
#include "libTSM/TSM.hpp"
#include "libnetwork/Connection.hpp"

struct IdentData{
    std::unordered_map<OblockID, int> oblockMap; 
    std::unordered_map<DeviceID, int> deviceMap; 
    std::unordered_map<int, DeviceID> intToDev; 
}; 


struct OutQueue{
    TSQ<OwnedSentMessage> &clientLine; 
    TSQ<OwnedSentMessage> outLine; 
    bool callbackRecieved = true; 

    OutQueue(TSQ<OwnedSentMessage> &osm)
    : clientLine(osm){}

    void insertMessage(OwnedSentMessage osm){
        if(callbackRecieved){
            this->callbackRecieved = false; 
            clientLine.write(osm);
            return; 
        }
        outLine.write(osm); 
    }   

    void updateCallback(){
        this->callbackRecieved = true; 
        if(!outLine.isEmpty()){
            this->clientLine.write(this->outLine.read()); 
        }
    }

}; 


// Client side EU
class ClientEU{
    private: 
        BlsLang::VirtualMachine virtualMachine; 
        int bytecodeOffset; 
        ReaderBox EuCache; 
        TSQ<EMStateMessage> reciever; 
        std::shared_ptr<DeviceScheduler> clientScheduler; 
        OblockID name; 
        OBlockDesc oinfo; 
        TSM<DeviceID, HeapMasterMessage> replacementCache; 
        int ctlCode; 
        std::unordered_map<DeviceID, int> devPosMap; 
        IdentData &idMaps; 
        std::vector<char> byteCodeSerialized; 
        std::unordered_map<DeviceID, std::unique_ptr<OutQueue>> &outLines; 
        
        void replaceCache(std::unordered_map<DeviceID, HeapMasterMessage> &currentLoad); 
        
    public: 
        ClientEU(OBlockDesc &odesc, std::shared_ptr<DeviceScheduler> scheObj, IdentData &idData, int ctlCode, std::vector<char> &bytecode, 
            std::unordered_map<DeviceID, std::unique_ptr<OutQueue>> &outLines);
        void insertDevice(HeapMasterMessage heapDesc);
        void run(std::stop_token st); 
        SentMessage createSentMessage(BlsType &hdesc, DeviceDescriptor &devDesc, Protocol pcode);
}; 

class ClientEM{
    private: 
        // Device to oblock list (using the in devices)
        std::unordered_map<DeviceID, std::vector<OblockID>> devToOblockMap; 
        BlsLang::VirtualMachine vmDivider; 

        // Add a loopback queue that is a heap descriptor by default
        std::shared_ptr<DeviceScheduler> clientScheduler; 
        
        // The connection outLine
        TSQ<OwnedSentMessage> &clientReadLine; 
        TSQ<OwnedSentMessage> &clientWriteLine; 
    
        int ctlCode; 
        std::unordered_map<OblockID, std::unique_ptr<ClientEU>> clientMap; 
        IdentData ident_data; 
        
        // Converts sent message to HMM
        HeapMasterMessage getHMM(SentMessage &toConvert, PROTOCOLS pcol);
        void beginVMs(); 
        bool isConfigured = false; 

        // Callback Enable Queue
        std::unordered_map<DeviceID, std::unique_ptr<OutQueue>> deviceOutMaps; 
        
        // Static function for converting HMM to SentMessage
        static SentMessage ownHmmConvert(); 

        std::vector<std::jthread> euThreads; 
    
    public: 
        ClientEM(
             TSQ<OwnedSentMessage> &readLine, 
             TSQ<OwnedSentMessage> &writeLine); 


        void run(std::stop_token st); 
        void config(std::vector<char> &bytecodeList, std::unordered_map<DeviceID, int> &deviceMap, int ctlCode); 
}; 


        