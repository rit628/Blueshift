#include "DeviceCore.hpp"

template<Driveable T>
void DeviceCore<T>::addFileIWatch(std::string &fileName, std::function<bool()> handler){
    hasInterrupt = true;
    this->Idesc_list.push_back({SrcType::UNIX_FILE, handler, fileName, 0}); 
}

template<Driveable T>
void DeviceCore<T>::addGPIOIWatch(int gpio_port, std::function<bool(int, int, uint32_t)> interruptHandle){
    hasInterrupt = true;
    this->Idesc_list.push_back({SrcType::GPIO, interruptHandle, "", gpio_port}); 
}

#define DEVTYPE_BEGIN(name) \
template class DeviceCore<TypeDef::name>;
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END