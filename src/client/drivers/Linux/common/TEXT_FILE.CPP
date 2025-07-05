#ifdef __linux__

#include "libnetwork/Connection.hpp"
#include "libnetwork/Protocol.hpp"
#include "libtype/typedefs.hpp"
#include "include/TEXT_FILE.hpp"
#include <fstream>

using namespace Device;

void Device::TEXT_FILE::processStates(DynamicMessage& dmsg) {
    TypeDef::TEXT_FILE query;
    dmsg.unpackStates(query);
    auto& operation = query.operation;
    std::fstream file(filename);
    file.seekg(query.index);
    if (operation.starts_with("READ")) {
        std::shared_lock readLock(m);
        for (int i = 0; i < std::stoi(operation.substr(5)); i++) {
            file >> query.readResult;
        }
    }
    else if (operation.starts_with("WRITE")) {
        std::unique_lock writeLock(m);
        auto writeOp= operation.substr(7);
        file.write(writeOp.c_str(), writeOp.size());
        query.writeResult = true;
    }
    else {
        throw BlsExceptionClass("TEXT_FILE: BAD COMMAND: " + operation, ERROR_T::DEVICE_FAILURE);
    }
    queryResults.write(query);
}

void Device::TEXT_FILE::init(std::unordered_map<std::string, std::string> &config) {
    this->filename = "./samples/client/" + config["file"];
    std::fstream file(filename);
    if(!file.is_open()){
        std::cout << "Could not find file" << std::endl;
        throw BlsExceptionClass("TEXT_FILE: " + this->filename, ERROR_T::BAD_DEV_CONFIG);
    }
}

void Device::TEXT_FILE::transmitStates(DynamicMessage &dmsg) {
    auto result = this->queryResults.read();
    dmsg.packStates(result);
}

Device::TEXT_FILE::~TEXT_FILE() {
    queryResults.clearQueue();
}

#endif