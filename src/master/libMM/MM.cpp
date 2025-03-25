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
            readTSQMap[OBlockList.at(i).name][deviceName] = TSQPtr;
            writeMap[deviceName] = make_unique<WriterBox>(deviceName);
        }
    }
    for(int i = 0; i < OBlockList.size(); i++)
    {
        for(int j = 0; j < OBlockList.at(i).binded_devices.size(); j++)
        {
            string deviceName = OBlockList.at(i).binded_devices.at(j).device_name;
            auto TSQptr = readTSQMap[OBlockList.at(i).name][deviceName];
            interruptMap[deviceName].push_back(TSQptr);
            interruptName_map[deviceName].push_back(OBlockList.at(i).name); 
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
            if(!writeMap.at(DMM.info.device)->waitingQ.isEmpty())
            {
                DynamicMasterMessage DMMtoSend = writeMap.at(DMM.info.device)->waitingQ.read();
                this->sendNM.write(DMMtoSend);
                cout << "Callback recieved on an nonempty Q" << endl;
            }
            else
            {
                writeMap.at(DMM.info.device)->waitingForCallback = false;
                cout << "Callback recieved on an empty Q" << endl;
            }
            //this->sendNM.write(DMMtoSend);
            // Diarreah
            //writeMap.at(DMM.info.device)->waitingForCallback = false;
            break;
        }
        case PROTOCOLS::SENDSTATES:
        {
            
            if(DMM.isInterrupt){
                for(int i = 0; i < interruptMap.at(DMM.info.device).size(); i++)
                { 
                    TSQ<DynamicMasterMessage> &interuptTSQ = *interruptMap.at(DMM.info.device).at(i);
                    // UPDATE THE OINFO DATA 
                    
                    // drop read should be true by default
                    bool dr = OBlockList.at(i).dropRead;
                    
                    if(dr){ 
                        std::cout<<"INTERRUPT DROP STATES RECEIED"<<std::endl;
                        DMM.info.oblock = interruptName_map.at(DMM.info.device).at(i);
                        interuptTSQ.clearQueue(); 
                        interuptTSQ.write(DMM);
                        this->oblockReadMap.at(DMM.info.oblock)->handleRequest(sendEM);
                    }
                    else{
                        DMM.info.oblock = interruptName_map.at(DMM.info.device).at(i);
                        interuptTSQ.write(DMM);
                        this->oblockReadMap.at(DMM.info.oblock)->handleRequest(sendEM);
                    }
                    
                  
                }
            }
            else{
                ReaderBox &assignedBox = *oblockReadMap.at(DMM.info.oblock);
                TSQ<DynamicMasterMessage> &assignedTSQ = *readTSQMap[DMM.info.oblock][DMM.info.device];

                if(assignedBox.dropRead == true)
                {
                    std::cout<<"Dropped read"<<std::endl;
                    assignedTSQ.clearQueue();
                    assignedTSQ.write(DMM);
                    assignedBox.handleRequest(sendEM);
                }
                else{
                    assignedTSQ.write(DMM);
                    assignedBox.handleRequest(sendEM);
                }
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
    ReaderBox &correspondingReaderBox = *oblockReadMap.at(DMM.info.oblock);
    switch(DMM.protocol)
    {
        case PROTOCOLS::REQUESTINGSTATES:
        {
            correspondingReaderBox.pending_requests = true; 
            break; 
        }
        case PROTOCOLS::SENDSTATES:
        {
            WriterBox &assignedBox = *writeMap.at(DMM.info.device);
            bool dropWrite = correspondingReaderBox.dropWrite;

            if(dropWrite == true && assignedBox.waitingForCallback == true)
            {
                cout << "Ignoring write" << endl;
                //assignedBox.waitingQ.write(DMM);
            }
            else if(dropWrite == false && assignedBox.waitingForCallback == true)
            {
                cout << "Storing message in Q until callback is recieved" << endl;
                assignedBox.waitingQ.write(DMM);
            }
            else if(dropWrite == false && assignedBox.waitingForCallback == false)
            {
                cout << "Send direct as callback was already recieved" << endl;
                this->sendNM.write(DMM);
                assignedBox.waitingForCallback = true;
            }
            else if(dropWrite == true && assignedBox.waitingForCallback == false)
            {
                cout << "Send direct as even though drop write is true it is not waiting for callback" << endl;
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
        std::cout<<"absorbed"<<std::endl;
        assignNM(currentDMM);
    }
}

void MasterMailbox::runningEM()
{
    while(1){
        DynamicMasterMessage currentDMM = this->readEM.read();
        assignEM(currentDMM);
    }
}
