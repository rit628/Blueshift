#include "EM.hpp"
#include "../libDM/DynamicMessage.hpp"
#include <gtest/gtest.h>

TEST(ExecutionUnitTest, ConstructorTest) 
{
    //Creates the devices
    vector<string> devices = {"DeviceA", "DeviceB"};
    vector<bool> isVtypes = {true, false};
    vector<string> controllers = {"CtrlA", "CtrlB"};
    
    //Creates the execution unit
    ExecutionUnit eu("BlockTest", devices, isVtypes, controllers);
    
    EXPECT_EQ(eu.OblockName, "BlockTest");
    ASSERT_EQ(eu.devices.size(), 2);
    EXPECT_EQ(eu.devices[0], "DeviceA");
    EXPECT_EQ(eu.controllers[1], "CtrlB");
}

TEST(ExecutionManagerTest, AssignMessage) 
{
    //Creates the DeviceDescriptors
    DeviceDescriptor dev;
    dev.device_name = "Device1";
    dev.devtype = DEVTYPE::SERVO;
    dev.controller = "Controller1";
    dev.isInterrupt = false;
    dev.isVtype = false;
    
    //Creates an oblock descriptor
    OBlockDesc block;
    block.name = "Block1";
    block.binded_devices.push_back(dev);
    block.bytecode_offset = 0;
    block.dropRead = false;
    block.dropWrite = false;
    
    vector<OBlockDesc> blocks = { block };

    TSQ<DynamicMasterMessage> in = TSQ<DynamicMasterMessage>();
    TSQ<HeapMasterMessage> out = TSQ<HeapMasterMessage>();

    //Creates a dynamic master message for the device
    DynamicMasterMessage dmm;
    dmm.info.oblock = "Block1";
    dmm.info.device = "Device1";
    dmm.info.controller = "Controller1";
    dmm.info.isVtype = false;
    
    //Creates the execution manager
    ExecutionManager em(blocks, in, out);
    
    //Call the assign function and make sure it assigns it to the right place
    ExecutionUnit &eu = em.assign(dmm);
    auto it = eu.stateMap.find("Device1");
    ASSERT_NE(it, eu.stateMap.end());
    EXPECT_EQ(it->second.info.oblock, dmm.info.oblock);
    EXPECT_EQ(it->second.info.device, dmm.info.device);
}

TEST(ExecutionUnitTest, ProcessUnit) 
{
    //Creates the execution units
    vector<string> devices = {"Device1"};
    vector<bool> isVtypes = {false};
    vector<string> controllers = {"Controller1"};
    ExecutionUnit eu("Block1", devices, isVtypes, controllers);
    
    //Creates a dynamic master message for the device
    DynamicMasterMessage dmm;
    dmm.info.oblock = "Block1";
    dmm.info.device = "Device1";
    dmm.info.controller = "Controller1";
    dmm.info.isVtype = false;
    eu.stateMap["Device1"] = dmm;
    
    //Calls process and make sure a heap master message is returned
    vector<HeapMasterMessage> results = eu.process(dmm, eu);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].info.device, "Device1");
}

TEST(ExecutionManagerTest, RunningNonVtypeMessage) 
{
    //Creates a nonvtype device
    DeviceDescriptor dev;
    dev.device_name = "Device1";
    dev.devtype = DEVTYPE::MOTOR;
    dev.controller = "Controller1";
    dev.isInterrupt = false;
    dev.isVtype = false;
    
    OBlockDesc block;
    block.name = "Block1";
    block.binded_devices.push_back(dev);
    block.bytecode_offset = 0;
    block.dropRead = false;
    block.dropWrite = false;
    
    vector<OBlockDesc> blocks = { block };

    TSQ<DynamicMasterMessage> in = TSQ<DynamicMasterMessage>();
    TSQ<HeapMasterMessage> out = TSQ<HeapMasterMessage>();

    //Creates a dynamic master message
    DynamicMasterMessage dmm;
    dmm.info.oblock = "Block1";
    dmm.info.device = "Device1";
    dmm.info.controller = "Controller1";
    dmm.info.isVtype = false;

    in.write(dmm);
    
    ExecutionManager em(blocks, in, out);
    
    //em.in.write(dmm);
    
    //Run the execution manager
    em.running(em.in);
    
    //Checks to see if the vtype vector is empty and the dmm makes it through the manager
    EXPECT_FALSE(em.out.isEmpty());
    HeapMasterMessage hmm = em.out.read();
    EXPECT_EQ(hmm.info.device, "Device1");
    EXPECT_TRUE(em.vtypeHMMs.empty());
}

TEST(ExecutionManagerTest, RunningVtypeMessage) 
{
    //Creates a device with a vtype
    DeviceDescriptor dev;
    dev.device_name = "DeviceV";
    dev.devtype = DEVTYPE::VINT;
    dev.controller = "ControllerV";
    dev.isInterrupt = false;
    dev.isVtype = true;
    
    OBlockDesc block;
    block.name = "BlockV";
    block.binded_devices.push_back(dev);
    block.bytecode_offset = 0;
    block.dropRead = false;
    block.dropWrite = false;
    
    vector<OBlockDesc> blocks = { block };

    TSQ<DynamicMasterMessage> in = TSQ<DynamicMasterMessage>();
    TSQ<HeapMasterMessage> out = TSQ<HeapMasterMessage>();

    //Creates a Dynamic Master Message
    DynamicMasterMessage dmm;
    dmm.info.oblock = "BlockV";
    dmm.info.device = "DeviceV";
    dmm.info.controller = "ControllerV";
    dmm.info.isVtype = true;

    in.write(dmm);
    
    ExecutionManager em(blocks, in, out);
    
    //em.in.write(dmm);
    
    //Runs the execution manager
    em.running(em.in);
    
    //Checks to see if the vtype vector is populated and the out queue is empty
    EXPECT_TRUE(em.out.isEmpty());
    ASSERT_EQ(em.vtypeHMMs.size(), 1);
    EXPECT_EQ(em.vtypeHMMs[0].info.device, "DeviceV");
}
