#include <memory>
#include <unordered_map>
#include <vector>
#include "Serialization.hpp"
#include "Scheduler.hpp"
#include "TSQ.hpp"
#include "Processing.hpp"
#include "Protocol.hpp"
#include "virtual_machine.hpp"
#include "bls_types.hpp"
#include "TSM.hpp"

struct IdentData{
    std::unordered_map<TaskID, int> taskMap; 
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
        DeviceScheduler &scheObj; 
        TaskID name; 
        TaskDescriptor taskInfo; 
        TSM<DeviceID, HeapMasterMessage> replacementCache; 
        TSQ<SentMessage> &clientMainLine; 
        int ctlCode; 
        std::unordered_map<DeviceID, int> devPosMap; 
        IdentData &idMaps; 
        
        void replaceCache(std::unordered_map<DeviceID, HeapMasterMessage> &currentLoad); 
        
    public: 
        ClientEU(TaskDescriptor &taskDesc, DeviceScheduler &scheObj, TSQ<SentMessage> &clientMainLine, 
        IdentData &idData);

        void insertDevice(HeapMasterMessage heapDesc);
        void run(); 
        SentMessage createSentMessage(BlsType &hdesc, DeviceDescriptor &devDesc, Protocol pcode);
}; 

class ClientEM{
    private: 
        // Device to task list (using the in devices)
        std::unordered_map<DeviceID, std::vector<TaskID>> devToTaskMap; 

        // Add a loopback queue that is a heap descriptor by default
        DeviceScheduler clientScheduler; 
        TSQ<SentMessage> &clientReadLine; 
        
        // The connection outLine
        TSQ<SentMessage> &clientWriteLine; 
    
        int ctlCode; 
        std::unordered_map<TaskID, std::unique_ptr<ClientEU>> clientMap; 
        IdentData ident_data; 

        // Converts sent message to HMM
        HeapMasterMessage getHMM(SentMessage &toConvert, PROTOCOLS pcol);
    
    public: 
        ClientEM(std::vector<TaskDescriptor> &descList, 
             TSQ<SentMessage> &readLine, 
             TSQ<SentMessage> &writeLine, 
             std::unordered_map<DeviceID, int> deviceMap, 
             int ctlCode); 

        void run(); 
}; 


        