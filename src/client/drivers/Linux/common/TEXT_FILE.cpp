#ifdef __linux__

#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "libtype/typedefs.hpp"
#include "include/TEXT_FILE.hpp"
#include <fstream>

using namespace Device;

void TEXT_FILE::processStates(DynamicMessage& dmsg) {
    TypeDef::TEXT_FILE query;
    dmsg.unpackStates(query);
    auto& operation = query.operation;
    std::fstream file(filename);
    if(!file.is_open()){
        std::cout << "File no longer available" << std::endl;
        throw BlsExceptionClass("TEXT_FILE: " + this->filename, ERROR_T::DEVICE_FAILURE);
    }
    file.seekg(query.index);
    if (operation.starts_with("READ")) {
        std::shared_lock readLock(m);
        auto numBytes = std::stoi(operation.substr(5));
        query.readResult.resize(numBytes);
        file.read(query.readResult.data(), numBytes);
    }
    else if (operation.starts_with("WRITE")) {
        std::unique_lock writeLock(m);
        auto writeOp= operation.substr(6);
        file.write(writeOp.c_str(), writeOp.size());
        query.writeResult = true;
    }
    else {
        std::cout<<"UNKWON COMMAND"<<std::endl; 
    }
    file.close();
    writeQueryResult(query);
}

void TEXT_FILE::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"];
    std::ofstream file(filename, ios::app);
    if(!file.is_open()){
        std::cout << "Could not find or create file" << std::endl;
        throw BlsExceptionClass("TEXT_FILE: " + this->filename, ERROR_T::BAD_DEV_CONFIG);
    }
    file.close();
}

void TEXT_FILE::transmitStates(DynamicMessage &dmsg) {
    auto lastState = getLastQueryResult();
    dmsg.packStates(lastState);
}

TEXT_FILE::~TEXT_FILE() {

}

#endif