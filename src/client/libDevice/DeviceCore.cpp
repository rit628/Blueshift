#include "DeviceCore.hpp"


template<Driveable T>
void DeviceCore<T>::addFileIWatch(std::string &fileName, std::function<bool()> handler) {
    hasInterrupt = true;
    this->Idesc_list.push_back(UnixFileInterruptor{fileName, handler}); 
}

template<Driveable T>
void DeviceCore<T>::addGPIOIWatch(uint8_t gpioPort, std::function<bool(int, int, uint32_t)> handler) {
    hasInterrupt = true;
    this->Idesc_list.push_back(GpioInterruptor{gpioPort, handler}); 
}


#ifdef SDL_ENABLED
template<Driveable T>
void DeviceCore<T>::addSDLIWatch(std::function<bool(SDL_Event*)> handler) {
    hasInterrupt = true;
    this->Idesc_list.push_back(SdlIoInterruptor{ handler}); 
}
#endif

#define DEVTYPE_BEGIN(name, ...) \
template class DeviceCore<TypeDef::name>;
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END