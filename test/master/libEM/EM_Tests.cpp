#include "EM.hpp"
#include "libDM/DynamicMessage.hpp"
#include "libHD/HeapDescriptors.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>
#include <optional>

// Test construction and field initialization of DynamicMasterMessage.
TEST(DynamicMasterMessageTest, ConstructorInitializesFields) {
    DynamicMessage dm;
    O_Info info { "OBlock1", "Device1", "Controller1", false };
    DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);
    
    EXPECT_EQ(dmm.info.oblock, "OBlock1");
    EXPECT_EQ(dmm.info.device, "Device1");
    EXPECT_EQ(dmm.info.controller, "Controller1");
    EXPECT_EQ(dmm.protocol, PROTOCOLS::SENDSTATES);
    EXPECT_FALSE(dmm.isInterrupt);
}

// Test construction and field initialization of HeapMasterMessage.
TEST(HeapMasterMessageTest, ConstructorInitializesFields) {
    auto heapTree = std::make_shared<MapDescriptor>(Desctype::ANY);
    O_Info info { "OBlock2", "Device2", "Controller2", true };
    HeapMasterMessage hmm(heapTree, info, PROTOCOLS::CALLBACKRECIEVED, true);
    
    EXPECT_EQ(hmm.info.oblock, "OBlock2");
    EXPECT_EQ(hmm.info.device, "Device2");
    EXPECT_EQ(hmm.info.controller, "Controller2");
    EXPECT_EQ(hmm.protocol, PROTOCOLS::CALLBACKRECIEVED);
    EXPECT_TRUE(hmm.isInterrupt);
    EXPECT_NE(hmm.heapTree, nullptr);
}

// Test ExecutionManager constructor creates ExecutionUnits as expected.
TEST(ExecutionManagerTest, ConstructorCreatesExecutionUnits) {
    // Create a dummy OBlockDesc.
    OBlockDesc oblockDesc;
    oblockDesc.name = "OBlock_Test";
    DeviceDescriptor device;
    device.device_name = "Device_Test";
    device.isVtype = false;
    device.controller = "Controller_Test";
    oblockDesc.binded_devices.push_back(device);
    
    vector<OBlockDesc> oblockList = { oblockDesc };
    
    // Create dummy TSQ objects.
    TSQ<vector<DynamicMasterMessage>> readQueue;
    TSQ<DynamicMasterMessage> sendQueue;
    
    // Construct ExecutionManager.
    ExecutionManager em(oblockList, readQueue, sendQueue);
    
    // Check that an ExecutionUnit was created in the map.
    auto it = em.EU_map.find("OBlock_Test");
    EXPECT_NE(it, em.EU_map.end());
    
    // Also check that the ExecutionUnit's device vector contains "Device_Test".
    ExecutionUnit &eu = *(it->second);
    ASSERT_FALSE(eu.devices.empty());
    EXPECT_EQ(eu.devices[0], "Device_Test");
}

// Test ExecutionManager::assign method properly assigns a DynamicMasterMessage.
TEST(ExecutionManagerTest, AssignAddsToExecutionUnitStateMap) {
    // Setup a dummy OBlockDesc and ExecutionManager.
    OBlockDesc oblockDesc;
    oblockDesc.name = "OBlock_Assign";
    DeviceDescriptor device;
    device.device_name = "Device_Assign";
    device.isVtype = false;
    device.controller = "Controller_Assign";
    oblockDesc.binded_devices.push_back(device);
    vector<OBlockDesc> oblockList = { oblockDesc };
    
    TSQ<vector<DynamicMasterMessage>> readQueue;
    TSQ<DynamicMasterMessage> sendQueue;
    ExecutionManager em(oblockList, readQueue, sendQueue);
    
    // Create a dummy DynamicMasterMessage.
    DynamicMessage dm;
    O_Info info { "OBlock_Assign", "Device_Assign", "Controller_Assign", false };
    DynamicMasterMessage dmm(dm, info, PROTOCOLS::REQUESTINGSTATES, false);
    
    // Call assign.
    ExecutionUnit &eu = em.assign(dmm);
    
    // Check that the ExecutionUnit's stateMap has the message under key "Device_Assign".
    auto found = eu.stateMap.find("Device_Assign");
    EXPECT_NE(found, eu.stateMap.end());
    EXPECT_EQ(found->second.protocol, PROTOCOLS::REQUESTINGSTATES);
}

TEST(ExecutionManagerTest, RunningProcessesReadQueue) {
    // Setup one OBlockDesc.
    OBlockDesc oblockDesc;
    oblockDesc.name = "OBlock_Running";
    DeviceDescriptor device;
    device.device_name = "Device_Running";
    device.isVtype = false;
    device.controller = "Controller_Running";
    oblockDesc.binded_devices.push_back(device);
    vector<OBlockDesc> oblockList = { oblockDesc };
    
    // Create dummy TSQs.
    TSQ<vector<DynamicMasterMessage>> readQueue;
    TSQ<DynamicMasterMessage> sendQueue;
    ExecutionManager em(oblockList, readQueue, sendQueue);
    
    // Prepare a vector with one DynamicMasterMessage.
    DynamicMessage dm;
    string strValue = "TestString";
    dm.createField("stringField", strValue);
    O_Info info { "OBlock_Running", "Device_Running", "Controller_Running", false };
    DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);
    vector<DynamicMasterMessage> dmmVec = { dmm };
    
    // Write the vector into the readQueue.
    readQueue.write(dmmVec);
    //readQueue.write(dmmVec);

    em.running(readQueue);

    std::this_thread::sleep_for(100ms);

    // The corresponding ExecutionUnit's EUcache should now contain the message.
    ExecutionUnit &eu = *em.EU_map.at("OBlock_Running");
    
    EXPECT_TRUE(eu.EUcache.isEmpty());

    eu.stop = true;
}

TEST(ExecutionManagerTest, RunningProcessesNonVtype) 
{
    OBlockDesc oblockDesc;
    oblockDesc.name = "OBlock_Running";
    DeviceDescriptor device;
    device.device_name = "Device_Running";
    device.isVtype = false;
    device.controller = "Controller_Running";
    oblockDesc.binded_devices.push_back(device);
    vector<OBlockDesc> oblockList = { oblockDesc };
    
    // Create dummy TSQs.
    TSQ<vector<DynamicMasterMessage>> readQueue;
    TSQ<DynamicMasterMessage> sendQueue;
    ExecutionManager em(oblockList, readQueue, sendQueue);

    // Prepare a vector with one DynamicMasterMessage.
    DynamicMessage dm;
    string strValue = "TestString";
    dm.createField("stringField", strValue);
    O_Info info { "OBlock_Running", "Device_Running", "Controller_Running", false };
    DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);
    vector<DynamicMasterMessage> dmmVec = { dmm };

    // Write the vector into the readQueue.
    readQueue.write(dmmVec);
    //readQueue.write(dmmVec);

    em.running(readQueue);

    std::this_thread::sleep_for(100ms);

    // The corresponding ExecutionUnit's EUcache should now contain the message.
    ExecutionUnit &eu = *em.EU_map.at("OBlock_Running");
    
    EXPECT_TRUE(eu.EUcache.isEmpty());
    EXPECT_EQ(em.sendMM.getSize(), 1);

    eu.stop = true;
}

TEST(ExecutionManagerTest, RunningProcessesVtype) 
{
    OBlockDesc oblockDesc;
    oblockDesc.name = "OBlock_Running";
    DeviceDescriptor device;
    device.device_name = "Device_Running";
    device.isVtype = true;
    device.controller = "Controller_Running";
    oblockDesc.binded_devices.push_back(device);
    vector<OBlockDesc> oblockList = { oblockDesc };
    
    // Create dummy TSQs.
    TSQ<vector<DynamicMasterMessage>> readQueue;
    TSQ<DynamicMasterMessage> sendQueue;
    ExecutionManager em(oblockList, readQueue, sendQueue);

    // Prepare a vector with one DynamicMasterMessage.
    DynamicMessage dm;
    string strValue = "TestString";
    dm.createField("stringField", strValue);
    O_Info info { "OBlock_Running", "Device_Running", "Controller_Running", true };
    DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);
    vector<DynamicMasterMessage> dmmVec = { dmm };

    // Write the vector into the readQueue.
    readQueue.write(dmmVec);
    //readQueue.write(dmmVec);

    em.running(readQueue);

    std::this_thread::sleep_for(100ms);

    // The corresponding ExecutionUnit's EUcache should now contain the message.
    ExecutionUnit &eu = *em.EU_map.at("OBlock_Running");
    
    EXPECT_TRUE(eu.EUcache.isEmpty());
    EXPECT_EQ(em.sendMM.getSize(), 0);

    eu.stop = true;
}

