#include "libcompiler/compiler.hpp"
#include "include/Common.hpp"
#include "libEM/EM.hpp"
#include "libMM/MM.hpp"
#include "libNM/MasterNM.hpp"
#include <functional>

using DMM = DynamicMasterMessage;


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
    std::vector<OBlockDesc> oblocks = compiler.getOblockDescriptors(); 
    auto functions = compiler.getOblocks();  

    // EM and MM
    TSQ<DMM> EM_MM_queue; 
    TSQ<std::vector<DMM>> MM_EM_queue; 

    DMM light_dmm;
    DeviceDescriptor dd = compiler.getDeviceDescriptors()["L1"];
    
    // NM and MM
    TSQ<DMM> NM_MM_queue; 
    TSQ<DMM> MM_NM_queue; 
    bool connections_made; 

    // Make network (runs at start)
    MasterNM NM(oblocks, MM_NM_queue, NM_MM_queue);
    NM.start(); 


    ExecutionManager EM(oblocks, MM_EM_queue, EM_MM_queue, functions); 
    std::thread t3([&](){EM.running();});
    
    // Make Mailbox (runs with EM and NM)
    MasterMailbox MM(oblocks, NM_MM_queue, EM_MM_queue, MM_NM_queue, MM_EM_queue); 
    std::thread t1([&](){MM.runningEM();}); 
    std::thread t2([&](){MM.runningNM();});   
    
    NM.makeBeginCall(); 

    t1.join(); 
    t2.join(); 
    t3.join(); 


    
    
}