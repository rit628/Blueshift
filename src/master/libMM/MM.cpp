#include "MM.hpp"
#include <memory>

MasterMailbox::MasterMailbox(vector<OBlockDesc> OBlockList, TSQ<DynamicMasterMessage> &readNM, 
    TSQ<DynamicMasterMessage> &readEM, TSQ<DynamicMasterMessage> &sendNM, TSQ<vector<DynamicMasterMessage>> &sendEM)
: readNM(readNM), readEM(readEM), sendNM(sendNM), sendEM(sendEM)
{
    this->OBlockList = OBlockList;
    for(int i = 0; i < OBlockList.size(); i++)
    {
        oblockReadMap[OBlockList.at(i).name] = make_unique<ReaderBox>(OBlockList.at(i).dropRead, 
        OBlockList.at(i).dropWrite, OBlockList.at(i).name);
        for(int j = 0; j < OBlockList.at(i).binded_devices.size(); j++)
        {
            string deviceName = OBlockList.at(i).binded_devices.at(j).device_name;
            auto TSQPtr = make_shared<TSQ<DynamicMasterMessage>>();
            oblockReadMap[OBlockList.at(i).name]->waitingQs.push_back(TSQPtr);
            readMap[OBlockList.at(i).name][deviceName] = TSQPtr;
            writeMap[deviceName] = make_unique<WriterBox>(deviceName);
        }
    }
    for(int i = 0; i < OBlockList.size(); i++)
    {
        for(int j = 0; j < OBlockList.at(i).binded_devices.size(); j++)
        {
            string deviceName = OBlockList.at(i).binded_devices.at(j).device_name;
            auto TSQptr = readMap[OBlockList.at(i).name][deviceName];
            interruptMap[deviceName].push_back(TSQptr);
        }
    }
}

ReaderBox::ReaderBox(bool dropRead, bool dropWrite, string name)
{
    this->dropRead = dropRead;
    this->dropWrite = dropWrite;
    this->OblockName = name;
}

WriterBox::WriterBox(string deviceName)
{
    this->deviceName = deviceName;
}

void MasterMailbox::assignNM(DynamicMasterMessage DMM)
{
    switch(DMM.protocol)
    {
        case PROTOCOLS::CALLBACKRECIEVED:
        {
            writeMap.at(DMM.info.device)->waitingForCallback = false;
            DynamicMasterMessage DMMtoSend = writeMap.at(DMM.info.device)->waitingQ.read();
            this->sendNM.write(DMMtoSend);
            writeMap.at(DMM.info.device)->waitingForCallback = true;
            break;
        }
        case PROTOCOLS::SENDSTATES:
        {
            ReaderBox &assignedBox = *oblockReadMap.at(DMM.info.oblock);
            TSQ<DynamicMasterMessage> &assignedTSQ = *readMap[DMM.info.oblock][DMM.info.device];
            if(assignedBox.dropRead == true)
            {
                assignedTSQ.clearQueue();
                assignedTSQ.write(DMM);
            }
            else if(DMM.isInterrupt == true)
            {
                for(int i = 0; i < interruptMap.at(DMM.info.device).size(); i++)
                {
                    TSQ<DynamicMasterMessage> &interuptTSQ = *interruptMap.at(DMM.info.device).at(i);
                    interuptTSQ.write(DMM);
                }
            }
            else
            {
                assignedTSQ.write(DMM);
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void MasterMailbox::assignEM(DynamicMasterMessage DMM)
{
    WriterBox &assignedBox = *writeMap.at(DMM.info.device);
    ReaderBox &correspondingReaderBox = *oblockReadMap.at(DMM.info.oblock);
    switch(DMM.protocol)
    {
        case PROTOCOLS::REQUESTINGSTATES:
        {
            int smallestTSQ = 10;
            for(int i = 0; i < correspondingReaderBox.waitingQs.size(); i++)
            {
                TSQ<DynamicMasterMessage> &currentTSQ = *correspondingReaderBox.waitingQs.at(i);
                if(currentTSQ.getSize() <= smallestTSQ){smallestTSQ = currentTSQ.getSize();}
            }
            for(int i = 0; i < smallestTSQ; i++)
            {
                vector<DynamicMasterMessage> statesToSend;
                for(int j = 0; j < correspondingReaderBox.waitingQs.size(); j++)
                {
                    statesToSend.push_back(correspondingReaderBox.waitingQs.at(j)->read());
                }
                this->sendEM.write(statesToSend);
            }
            break;
        }
        case PROTOCOLS::SENDSTATES:
        {
            bool dropWrite = correspondingReaderBox.dropWrite;
            if(assignedBox.waitingForCallback == false && dropWrite == false)
            {   
                this->sendNM.write(DMM);
                assignedBox.waitingForCallback = true;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void MasterMailbox::runningNM()
{
    while(1)
    {
        DynamicMasterMessage currentDMM = this->readNM.read();
        assignNM(currentDMM);
    }
}

void MasterMailbox::runningEM()
{
    while(1)
    {
        DynamicMasterMessage currentDMM = this->readEM.read();
        assignEM(currentDMM);
    }
}
