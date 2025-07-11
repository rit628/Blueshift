#include "libcompiler/compiler.hpp"
#include "include/Common.hpp"
#include "libEM/EM.hpp"
#include "libMM/MM.hpp"
#include "libNM/MasterNM.hpp"
#include "libtype/bls_types.hpp"
#include <functional>
#include <unordered_map>
#include <vector>


using DMM = DynamicMasterMessage;



/*
    Modifies the Oblock Descriptor with the data derived from the 
    dependency graph. 
*/

// void modifyOblockDesc(std::vector<OBlockDesc> &oDescs){
//     std::unordered_map<DeviceID, DeviceDescriptor&> devDesc; 
//     for(auto& oblock : oDescs){
//         // Construct the device device desc map: 
//         std::unordered_map<DeviceID, DeviceDescriptor> devMap; 
//         for(auto& str : oblock.binded_devices){
//             devMap[str.device_name] = str; 
//         }
        
//         // Remove the artifical oblock adder
//         if(oblock.name == "task"){
//             std::cout<<"Establishing trigger rules"<<std::endl; 
//             oblock.customTriggers = true; 
//             oblock.triggerRules = {{"lw", "rfp"}, {"lw"}}; 
//         }

//     }
// }


// Temporary fill until the divider is filly implemented
void modifyOblockDesc(std::vector<OBlockDesc> &oDescs,  GlobalContext gcx){
    std::unordered_map<DeviceID, DeviceDescriptor&> devDesc; 
    for(auto& oblock : oDescs){
        // Construct the device device desc map: 
        std::unordered_map<DeviceID, DeviceDescriptor> devMap; 
        for(auto& str : oblock.binded_devices){
            devMap[str.device_name] = str; 
        }

        // Fill in the global context: 
        auto& inMap = gcx.oblockConnections[oblock.name].inDeviceList; 
        auto& outMap = gcx.oblockConnections[oblock.name].outDeviceList; 

        for(auto &devId : inMap){
            oblock.inDevices.push_back(devMap[devId]);
        }

        for(auto& devId : outMap){
            oblock.outDevices.push_back(devMap[devId]); 
        }   
    }
}


int main(int argc, char *argv[]){

    
    std::string filename;

    if(argc == 2){
        filename = std::string(std::string(argv[1])); 
    }
    else{
        std::cout<<"Invalid number of arguments"<<std::endl;
        return 1;
    }
    
    printf("pre compilation\n");
    // Makes interpreter
    std::vector<char> bytecode;
    BlsLang::Compiler compiler;
    compiler.compileFile(filename, bytecode); 
    printf("past compilation\n");

    std::vector<OBlockDesc> oblockDescriptors = compiler.getOblockDescriptors(); 
    auto oblocks = compiler.getOblocks();  

    // Only temporary until symgraph is complete
    modifyOblockDesc(oblockDescriptors, compiler.getGlobalContext()); 
    

    // EM and MM
    TSQ<HeapMasterMessage> EM_MM_queue; 
    TSQ<EMStateMessage> MM_EM_queue; 
    
    // NM and MM
    TSQ<DMM> NM_MM_queue; 
    TSQ<DMM> MM_NM_queue; 

    // Make network (runs at start)
    MasterNM NM(oblockDescriptors, MM_NM_queue, NM_MM_queue);
    NM.start(); 


    ExecutionManager EM(oblockDescriptors, MM_EM_queue, EM_MM_queue, bytecode, oblocks); 
    std::thread t3([&](){EM.running();});
    
    // Make Mailbox (runs with EM and NM)
    MasterMailbox MM(oblockDescriptors, NM_MM_queue, EM_MM_queue, MM_NM_queue, MM_EM_queue); 
    std::thread t1([&](){MM.runningEM();}); 
    std::thread t2([&](){MM.runningNM();});   
    
    NM.makeBeginCall(); 

    t1.join(); 
    t2.join(); 
    t3.join(); 
    
    
}