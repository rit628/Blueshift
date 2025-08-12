#include "DeviceCore.hpp"
#include "include/HttpListener.hpp"
#include <memory>
#include <thread>

#define HTTP_PORT 8080

template<Driveable T>
void DeviceCore<T>::addFileIWatch(std::string &fileName, std::function<bool()> handler) {
    this->Idesc_list.push_back(UnixFileInterruptor{fileName, handler}); 
}

template<Driveable T>
void DeviceCore<T>::addGPIOIWatch(uint8_t gpioPort, std::function<bool(int, int, uint32_t)> handler) {
    this->Idesc_list.push_back(GpioInterruptor{gpioPort, handler}); 
}

#ifdef SDL_ENABLED
template<Driveable T>
void DeviceCore<T>::addSDLIWatch(std::function<bool(SDL_Event*)> handler) {
    this->Idesc_list.push_back(SdlIoInterruptor{ handler}); 
}
#endif

template<Driveable T> 
std::shared_ptr<HttpListener> DeviceCore<T>::addEndpointIWatch(std::string ep, std::function<bool(int, std::string, std::string)> handler){
    
    auto king = HttpListener::createServer(HTTP_PORT); 
    std::cout<<"adding ep: "<<ep<<std::endl; 
    this->Idesc_list.push_back(HttpInterruptor{.server = king, .endpoint = ep, .interruptCallback=handler, }); 
    return king; 
}

template<Driveable T>
void DeviceCore<T>::writeQueryResult(T& states) {
    std::pair<std::thread::id, T> queryResult = {std::this_thread::get_id(), states};
    queryQueue.write(queryResult);
}

template<Driveable T>
T DeviceCore<T>::getLastQueryResult() {
    auto result = this->queryQueue.pop();
    if (result.has_value()) {
        return result.value().second;
    }
    else {
        return states;
    }
}

#define DEVTYPE_BEGIN(name, ...) \
template class DeviceCore<TypeDef::name>;
#define ATTRIBUTE(...)
#define DEVTYPE_END
#include "DEVTYPES.LIST"
#undef DEVTYPE_BEGIN 
#undef ATTRIBUTE
#undef DEVTYPE_ENDs