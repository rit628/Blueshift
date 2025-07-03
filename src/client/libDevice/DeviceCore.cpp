#include "DeviceCore.hpp"

template<Driveable T>
void DeviceCore<T>::addFileIWatch(std::string &fileName, std::function<bool()> handler) {
    hasInterrupt = true;
    this->Idesc_list.push_back({SrcType::UNIX_FILE, handler, fileName, 0}); 
}

template<Driveable T>
void DeviceCore<T>::addGPIOIWatch(int gpio_port, std::function<bool(int, int, uint32_t)> handler) {
    hasInterrupt = true;
    this->Idesc_list.push_back({SrcType::GPIO, handler, "", gpio_port}); 
}

#ifdef SDL_ENABLED
template<Driveable T>
void DeviceCore<T>::addSDLIWatch(std::function<bool(SDL_Event*)> handler) {
    hasInterrupt = true;
    this->Idesc_list.push_back({SrcType::SDL_IO, handler,"", 0}); 
}
#endif

#define DEVTYPE_BEGIN(name) \
template class DeviceCore<TypeDef::name>;
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN
#undef ATTRIBUTE
#undef DEVTYPE_END