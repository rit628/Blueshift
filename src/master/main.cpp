#include "libcompiler/compiler.hpp"
#include "include/Common.hpp"
#include "libEM/EM.hpp"
#include "libMM/MM.hpp"
#include "libNM/MasterNM.hpp"
#include "libdepgraph/depgraph.hpp"
#include "libtypes/bls_types.hpp"
#include <functional>
#include <unordered_map>

using DMM = DynamicMasterMessage;


/*
Dummy temp function to fill in the arguments 
into the device descriptors depending on the 
devtype (THIS SHOULD BE REMOVED WHEN THE DEVICE BINDING 
METADATA IS PARSED IN THE LANGUAGE)
*/ 

void filterDevice(DeviceDescriptor &devDesc){
    switch(devDesc.devtype){
        case(DEVTYPE::TIMER_TEST) : {
            devDesc.polling_period = 2000; 
            break; 
        }
        case(DEVTYPE::FILE_LOG): {
            devDesc.isTrigger = false; 
            break; 
        }
        case(DEVTYPE::READ_FILE) : {   
            std::cout<<"Set READ FILE"<<std::endl; 

            devDesc.isTrigger = false; 
            devDesc.dropRead = false; 
            break;
        }
        case(DEVTYPE::LINE_WRITER) : {
            devDesc.isTrigger = true; 
            break; 
        }
    }
}


void alterBindedDevices(std::vector<OBlockDesc> &descriptors){
    for(auto& oblock : descriptors){
        for(auto& devices : oblock.inDevices){
            filterDevice(devices); 
        } 
        for(auto& devices : oblock.outDevices){
            filterDevice(devices); 
        }
        for(auto& devices: oblock.binded_devices){
            filterDevice(devices); 
        }
    }

}


/*
    Modifies the Oblock Descriptor with the data derived from the 
    dependency graph. 
*/

void modifyOblockDesc(std::vector<OBlockDesc> &oDescs,  GlobalContext &gcx){
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
    BlsLang::Compiler compiler;
    compiler.compileFile(filename); 
    printf("past compilation\n");

    //auto oblocks = interpreter.getOblockDescriptors();
    std::vector<OBlockDesc> oblockDescriptors = compiler.getOblockDescriptors(); 
    auto oblocks = compiler.getOblocks();  

    // EM and MM
    TSQ<DMM> EM_MM_queue; 
    TSQ<std::vector<DMM>> MM_EM_queue; 
    
    // NM and MM
    TSQ<DMM> NM_MM_queue; 
    TSQ<DMM> MM_NM_queue; 

    // Make network (runs at start)
    MasterNM NM(oblockDescriptors, MM_NM_queue, NM_MM_queue);
    NM.start(); 


    ExecutionManager EM(oblockDescriptors, MM_EM_queue, EM_MM_queue, oblocks); 
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