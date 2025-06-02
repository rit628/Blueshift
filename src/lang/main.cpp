#include "include/Common.hpp"
#include "libcompiler/compiler.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <bitset>

// Use a bitmap to quickly evaluate trigger rules. 
// Assuming hte 

int main() {


    // sample engine
    BlsLang::Lexer lexer;
    std::string test = \
    R"(a bunch of text on multiple
    lines with some // comments
    that are //singleline and
    multiline as follows /* some multiline
    comment that finishes just
    about here */ now after that lest test some
    strings like this one "some text \n \t " now
    time for some numbers 1 12 -12 decimals 
    too 1.2 .1 -1.2 -.3 -0.5 hooray now time for 
    the ops <= >= != == += -= *= /= ^= %= ++ -- && || . () $ yipee!)";

    /*
    BlsLang::Compiler compiler;
    compiler.compileFile("samples/src/rpi.blu");


    /*
    for (auto&& i : compiler.getOblockDescriptors()) {
        std::cout << i.name << std::endl;
        for (auto&& j : i.binded_devices) {
            std::cout << j.device_name << std::endl;
            std::cout << j.controller << std::endl;
            std::cout << static_cast<int>(j.devtype) << std::endl;
            for (auto&& k : j.port_maps) {
                std::cout << k.first << std::endl;
                std::cout << k.second << std::endl;
            }
        }
    }
    */ 

    // std::ifstream file;
    // file.open("test/lang/samples/simple.blu");
    // std::stringstream ss;
    // ss << file.rdbuf();
    // auto ret = lexer.lex(ss.str());
    // std::cout << ret.size() << std::endl;
    // for (auto&& i : ret) {
    //     std::cout << i.getTypeName() << " " << i.getLiteral() << " @ " << i.getLineNum() << ":" << i.getColNum() << " or " << i.getAbsIdx() <<  std::endl;
    // }

    // BlsLang::Parser parser;
    // auto ast = parser.parse(ret);

    // std::cout << *ast;
    
    // BlsLang::Interpreter interpreter;
    // ast->accept(interpreter);
    // for (auto&& i : interpreter.getOblockDescriptors()) {
    //     std::cout << i.name << std::endl;
    //     for (auto&& j : i.binded_devices) {
    //         std::cout << j.device_name << std::endl;
    //         std::cout << j.controller << std::endl;
    //         std::cout << static_cast<int>(j.devtype) << std::endl;
    //         for (auto&& k : j.port_maps) {
    //             std::cout << k.first << std::endl;
    //             std::cout << k.second << std::endl;
    //         }
    //     }
    // }
    /*
    DeviceDescriptor d1; 
    DeviceDescriptor d2; 
    DeviceDescriptor d3; 

    d1.device_name = "dev1"; 
    d2.device_name = "dev2"; 
    d3.device_name = "dev3"; 

    OBlockDesc testDesc; 
    testDesc.customTriggers = true;
    testDesc.inDevices = {d1, d2, d3}; 
    testDesc.triggerRules = {{"dev1"}}; 
    
    TriggerManager obj(testDesc); 
    obj.debugPrintRules(); 

    bool dev1Placement = obj.processDevice(d1.device_name); 
    bool dev2Placement = obj.processDevice(d2.device_name); 
    std::cout<<"dev1: "<<dev1Placement<<std::endl; 
    std::cout<<"dev2: "<<dev2Placement<<std::endl; 

    bool dev1p2 = obj.processDevice(d1.device_name); 
    std::cout<<"dev1p2: "<<dev1p2<<std::endl; 
    bool dev3 = obj.processDevice(d3.device_name); 
    std::cout<<"dev3: "<<dev3<<std::endl; 

    bool omar2 = obj.processDevice(d2.device_name); 
    bool omar1 = obj.processDevice(d1.device_name); 

    std::cout<<"omar2: "<<omar2<<std::endl; 
    std::cout<<"omar1: "<<omar1<<std::endl; 
    */ 


    



   













    return 0;

    
}