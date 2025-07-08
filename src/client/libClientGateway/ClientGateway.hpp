#include <iostream>
#include <memory>
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

struct IdentData{
    std::unordered_map<OblockID, int> oblockMap; 
    std::unordered_map<DeviceID, int> deviceMap; 
    std::unordered_map<int, DeviceID> intToDev; 
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
        TSQ<SentMessage> &clientMainLine; 
        int ctlCode; 
        std::unordered_map<DeviceID, int> devPosMap; 
        IdentData &idMaps; 
        std::vector<char> byteCodeSerialized; 
        
        void replaceCache(std::unordered_map<DeviceID, HeapMasterMessage> &currentLoad); 
        
    public: 
        ClientEU(OBlockDesc &odesc, std::shared_ptr<DeviceScheduler> scheObj, TSQ<SentMessage> &clientMainLine,  IdentData &idData, int ctlCode, std::vector<char> &bytecode);

        void insertDevice(HeapMasterMessage heapDesc);
        void run(); 
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
        TSQ<SentMessage> &clientReadLine; 
        TSQ<SentMessage> &clientWriteLine; 
    
        int ctlCode; 
        std::unordered_map<OblockID, std::unique_ptr<ClientEU>> clientMap; 
        IdentData ident_data; 
        
        // Converts sent message to HMM
        HeapMasterMessage getHMM(SentMessage &toConvert, PROTOCOLS pcol);
    
    public: 
        ClientEM(
            std::vector<char> &bytecodeList,
             TSQ<SentMessage> &readLine, 
             TSQ<SentMessage> &writeLine, 
             std::unordered_map<DeviceID, int> deviceMap, 
             int ctlCode); 



        

        void run(); 
}; 


        