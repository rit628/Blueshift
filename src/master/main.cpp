#include "compiler.hpp"
#include "Serialization.hpp"
#include "EM.hpp"
#include "MM.hpp"
#include "MasterNM.hpp"
#include "bls_types.hpp"
#include <functional>
#include <unordered_map>
#include <vector>


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
    std::vector<char> bytecode;
    BlsLang::Compiler compiler;
    compiler.compileFile(filename, bytecode); 
    
    printf("past compilation\n");

    std::vector<TaskDescriptor> taskDescriptors = compiler.getTaskDescriptors(); 
    auto tasks = compiler.getTasks();  
    

    // EM and MM
    TSQ<HeapMasterMessage> EM_MM_queue; 
    TSQ<EMStateMessage> MM_EM_queue; 
    
    // NM and MM
    TSQ<DMM> NM_MM_queue; 
    TSQ<DMM> MM_NM_queue; 

    // Make network (runs at start)
    MasterNM NM(taskDescriptors, MM_NM_queue, NM_MM_queue);
    NM.start(); 


    ExecutionManager EM(taskDescriptors, MM_EM_queue, EM_MM_queue, bytecode, tasks); 
    std::thread t3([&](){EM.running();});
    
    // Make Mailbox (runs with EM and NM)
    MasterMailbox MM(taskDescriptors, NM_MM_queue, EM_MM_queue, MM_NM_queue, MM_EM_queue); 
    std::thread t1([&](){MM.runningEM();}); 
    std::thread t2([&](){MM.runningNM();});   
    
    NM.makeBeginCall(); 

    t1.join(); 
    t2.join(); 
    t3.join(); 
    
    
}