// #include "MM.hpp"
// #include "libEM/EM.hpp"
// #include "libEM/EM.cpp"
// #include "libDM/DynamicMessage.hpp"
// #include <gtest/gtest.h>
// #include <thread>

// TEST(MasterMailboxBlackBoxTest, RunningNM_ConsumesInputQueue) 
// {
//     TSQ<DynamicMasterMessage> readNM;
//     TSQ<DynamicMasterMessage> readEM;

//     OBlockDesc oblockDesc;
//     oblockDesc.name = "OB_ConsumeNM";
//     oblockDesc.dropRead = false;
//     oblockDesc.dropWrite = false;
//     DeviceDescriptor device;
//     device.device_name = "device_consumeNM";
//     device.isVtype = false;
//     device.controller = "ctrl";
//     oblockDesc.binded_devices.push_back(device);
    
//     vector<OBlockDesc> oblockList = { oblockDesc };

//     MasterMailbox mailbox(oblockList, readNM, readEM);
//     DynamicMessage dm;
//     O_Info info { "OB_ConsumeNM", "device_consumeNM", "ctrl", false };
//     DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);

//     readNM.write(dmm);
//     readNM.write(dmm);

//     mailbox.runningNM(readNM);
//     EXPECT_TRUE(readNM.isEmpty());
// }

// TEST(MasterMailboxBlackBoxTest, RunningEM_ConsumesInputQueue) 
// {
//     TSQ<DynamicMasterMessage> readNM;
//     TSQ<DynamicMasterMessage> readEM;

//     OBlockDesc oblockDesc;
//     oblockDesc.name = "OB_ConsumeEM";
//     oblockDesc.dropRead = false;
//     oblockDesc.dropWrite = false;
//     DeviceDescriptor device;
//     device.device_name = "device_consumeEM";
//     device.isVtype = false;
//     device.controller = "ctrl";
//     oblockDesc.binded_devices.push_back(device);
    
//     vector<OBlockDesc> oblockList = { oblockDesc };

//     MasterMailbox mailbox(oblockList, readNM, readEM);
//     DynamicMessage dm;
//     O_Info info { "OB_ConsumeEM", "device_consumeEM", "ctrl", false };
//     DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);

//     readEM.write(dmm);
//     mailbox.runningEM(readEM);
//     EXPECT_TRUE(readEM.isEmpty());
// }

// TEST(MasterMailboxBlackBoxTest, RunningEM_ValidSendStatesProcesses) 
// {
//     TSQ<DynamicMasterMessage> readNM;
//     TSQ<DynamicMasterMessage> readEM;

//     OBlockDesc oblockDesc;
//     oblockDesc.name = "OB_EM_SendStates";
//     oblockDesc.dropRead = false;
//     oblockDesc.dropWrite = false;
//     DeviceDescriptor device;
//     device.device_name = "device_em_send";
//     device.isVtype = false;
//     device.controller = "ctrl";
//     oblockDesc.binded_devices.push_back(device);
    
//     vector<OBlockDesc> oblockList = { oblockDesc };

//     MasterMailbox mailbox(oblockList, readNM, readEM);
//     DynamicMessage dm;
//     O_Info info { "OB_EM_SendStates", "device_em_send", "ctrl", false };
//     DynamicMasterMessage dmm(dm, info, PROTOCOLS::SENDSTATES, false);

//     readEM.write(dmm);
//     mailbox.runningEM(readEM);

//     EXPECT_EQ(mailbox.sendNM.getSize(), 1);
//     EXPECT_TRUE(readEM.isEmpty());
// }

// TEST(MasterMailboxBlackBoxTest, RunningEM_RequestingStates) 
// {
//     TSQ<DynamicMasterMessage> readNM;
//     TSQ<DynamicMasterMessage> readEM;

//     OBlockDesc oblockDesc;
//     oblockDesc.name = "OB_RequestStates";
//     oblockDesc.dropRead = false;
//     oblockDesc.dropWrite = false;
//     DeviceDescriptor device;
//     device.device_name = "device_request";
//     device.isVtype = false;
//     device.controller = "ctrl";
//     oblockDesc.binded_devices.push_back(device);
    
//     vector<OBlockDesc> oblockList = { oblockDesc };

//     MasterMailbox mailbox(oblockList, readNM, readEM);

//     DynamicMessage dm;
//     O_Info info { "OB_RequestStates", "device_request", "ctrl", false };
//     O_Info infoTwo { "OB_RequestStates", "device2_request", "ctrl", false };
//     DynamicMasterMessage stateMsgOne(dm, info, PROTOCOLS::SENDSTATES, false);
//     DynamicMasterMessage stateMsgTwo(dm, infoTwo, PROTOCOLS::SENDSTATES, false);

//     readNM.write(stateMsgOne);
//     readNM.write(stateMsgOne);
//     mailbox.runningNM(readNM);

//     DynamicMasterMessage requestStates(dm, info, PROTOCOLS::REQUESTINGSTATES, false);

//     readEM.write(requestStates);
//     mailbox.runningEM(readEM);

//     std::vector<DynamicMasterMessage> aggregated = mailbox.sendEM.read();
//     EXPECT_EQ(aggregated.size(), 1);
//     EXPECT_TRUE(readEM.isEmpty());
// }

// TEST(MasterMailboxBlackBoxTest, RunningEM_RequestingTwoStates) 
// {
//     TSQ<DynamicMasterMessage> readNM;
//     TSQ<DynamicMasterMessage> readEM;

//     OBlockDesc oblockDesc;
//     oblockDesc.name = "OB_RequestStates";
//     oblockDesc.dropRead = false;
//     oblockDesc.dropWrite = false;
//     DeviceDescriptor device;
//     device.device_name = "device_request";
//     device.isVtype = false;
//     device.controller = "ctrl";
//     oblockDesc.binded_devices.push_back(device);
//     DeviceDescriptor deviceTwo;
//     deviceTwo.device_name = "device2_request";
//     deviceTwo.isVtype = false;
//     deviceTwo.controller = "ctrl";
//     oblockDesc.binded_devices.push_back(deviceTwo);
    
//     vector<OBlockDesc> oblockList = { oblockDesc };

//     MasterMailbox mailbox(oblockList, readNM, readEM);

//     DynamicMessage dm;
//     O_Info info { "OB_RequestStates", "device_request", "ctrl", false };
//     O_Info infoTwo { "OB_RequestStates", "device2_request", "ctrl", false };
//     DynamicMasterMessage stateMsgOne(dm, info, PROTOCOLS::SENDSTATES, false);
//     DynamicMasterMessage stateMsgTwo(dm, infoTwo, PROTOCOLS::SENDSTATES, false);

//     readNM.write(stateMsgOne);
//     readNM.write(stateMsgOne);
//     readNM.write(stateMsgTwo);
//     readNM.write(stateMsgTwo);
//     mailbox.runningNM(readNM);

//     DynamicMasterMessage requestStates(dm, info, PROTOCOLS::REQUESTINGSTATES, false);

//     readEM.write(requestStates);
//     mailbox.runningEM(readEM);

//     vector<DynamicMasterMessage> aggregated = mailbox.sendEM.read();
//     EXPECT_EQ(aggregated.size(), 2);
//     EXPECT_TRUE(readEM.isEmpty());
// }

// //Create a test for the interrupts
// TEST(MasterMailboxMultiDeviceTest, InterruptHandlingAcrossMultipleOBlocks) {
//     TSQ<DynamicMasterMessage> readNM;
//     TSQ<DynamicMasterMessage> readEM;
    
//     // Create two different OBlocks; each has one device with the same device name.
//     OBlockDesc block1;
//     block1.name = "OB_1";
//     block1.dropRead = false;
//     block1.dropWrite = false;
//     block1.bytecode_offset = 0;
//     DeviceDescriptor commonDev1;
//     commonDev1.device_name = "common_device";
//     commonDev1.devtype = DEVTYPE::BUTTON;
//     commonDev1.controller = "ctrlA";
//     commonDev1.port_maps = {};
//     commonDev1.isInterrupt = false;
//     commonDev1.isVtype = false;
//     block1.binded_devices.push_back(commonDev1);
    
//     OBlockDesc block2;
//     block2.name = "OB_2";
//     block2.dropRead = false;
//     block2.dropWrite = false;
//     block2.bytecode_offset = 0;
//     DeviceDescriptor commonDev2;
//     commonDev2.device_name = "common_device";
//     commonDev2.devtype = DEVTYPE::BUTTON;
//     commonDev2.controller = "ctrlB";
//     commonDev2.port_maps = {};
//     commonDev2.isInterrupt = false;
//     commonDev2.isVtype = false;
//     block2.binded_devices.push_back(commonDev2);
    
//     std::vector<OBlockDesc> blocks = { block1, block2 };
//     MasterMailbox mailbox(blocks, readNM, readEM);
    
//     // Create a SENDSTATES message with isInterrupt true for the common device.
//     // (For the interrupt branch the oblock field in the message is not used to select the queues.)
//     DynamicMessage dm;
//     O_Info info = { "OB_1", "common_device", "ctrlA", false };
//     DynamicMasterMessage intMsg(dm, info, PROTOCOLS::SENDSTATES, true);
    
//     // Write the interrupt message into readNM.
//     readNM.write(intMsg);
    
//     mailbox.runningNM(readNM);
    
//     // In the constructor, each OBlock that contains the common device registers its TSQ into the
//     // interruptMap under key "common_device". Thus, we expect two TSQs in the vector.
//     auto& intQueues = mailbox.interruptMap["common_device"];
//     EXPECT_EQ(intQueues.size(), 2);
    
//     // Each interrupt queue should have received the message.
//     for (auto& tsqPtr : intQueues) {
//         EXPECT_EQ(tsqPtr->getSize(), 1);
//         DynamicMasterMessage res = tsqPtr->read();
//         EXPECT_EQ(res.info.device, "common_device");
//         EXPECT_TRUE(res.isInterrupt);
//     }
    
//     EXPECT_TRUE(readNM.isEmpty());
// }