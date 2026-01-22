#include "traps.hpp"
#include "libbytecode/bytecode_processor.hpp"
#include "EM.hpp"
#include "libtype/bls_types.hpp"
#include <chrono>
#include <concepts>
#include <exception>
#include <memory>
#include <ranges>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include <boost/range/iterator_range_core.hpp>
#include <boost/beast.hpp> 
#include <boost/asio.hpp>

using namespace BlsTrap;
namespace asio = boost::asio; 
namespace beast = boost::beast; 
namespace http = beast::http; 
namespace ssl = asio::ssl;

std::monostate Impl::print(std::vector<BlsType> values, BytecodeProcessor* vm) {
    if (values.size() > 0) {
        std::cout << values.at(0) << std::flush;
        for (auto&& arg : boost::make_iterator_range(values.begin()+1, values.end())) {
            std::cout << " " << arg << std::flush;
        }
        std::cout << std::endl;
    }
    return std::monostate();
}

std::monostate Impl::println(std::vector<BlsType> values, BytecodeProcessor* vm) {
    for (auto&& value : values) {
        std::cout << value << std::endl;
    }
    return std::monostate();
}

std::monostate Impl::sleep(int64_t ms, BytecodeProcessor* vm) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return std::monostate();
}

int64_t Impl::stoi(std::string input, BytecodeProcessor* vm) {
    return std::stoi(input);
}

std::string Impl::toString(double input, BytecodeProcessor* vm) {
    // use a stringstream for now for better formatted output; eventually overloads will allow for std::to_string
    std::stringstream out;
    out << input;
    return out.str();
}

std::string Impl::getTrigger(BytecodeProcessor* vm) {
    if (!vm) return "";
    return vm->ownerUnit->TriggerName;
}

std::string Impl::time(BytecodeProcessor* vm) {
    std::time_t result = std::time(nullptr);
    return std::asctime(std::localtime(&result));
}


std::monostate Impl::disableTrigger(std::string triggerName, std::string taskID, BytecodeProcessor* vm){
    vm->ownerUnit->sendTriggerChange(triggerName, taskID, false); 
    return std::monostate();
}

std::monostate Impl::enableTrigger(std::string triggerName, std::string taskID, BytecodeProcessor* vm){
    vm->ownerUnit->sendTriggerChange(triggerName, taskID, true); 
    return std::monostate(); 
}

std::monostate Impl::push(std::vector<BlsType> blsList, BytecodeProcessor *vm){
    // imagine a push function here
    vm->ownerUnit->sendPushState(blsList); 
    return std::monostate(); 
}

std::monostate Impl::pull(std::vector<BlsType> blsList, BytecodeProcessor *vm){
    auto newStates = vm->ownerUnit->sendPullState(blsList); 
    int i = 0; 
    for(auto oldVal : blsList){
        if(std::holds_alternative<std::shared_ptr<HeapDescriptor>>(oldVal)){
            auto old_hd = std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(oldVal));
            auto new_hd  = std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(newStates.at(i)));
            new_hd->index = old_hd->index; 
            *old_hd = *new_hd; 
            std::cout<<"Switched results"<<std::endl; 
        }
        else{
            //  Wont work for now since blsList is copied by value
            oldVal = blsList.at(i); 
        }
        i++; 
    }

    return std::monostate(); 
}

std::monostate Impl::sync(std::vector<BlsType> blsList, BytecodeProcessor *vm){
    Impl::push(blsList); 
    return Impl::pull(blsList);
}

BlsType Impl::loadJson(std::string json, BytecodeProcessor* vm) {
    using namespace boost::json;
    return value_to<BlsType>(parse(json));
}

std::string Impl::jsonify(BlsType value, BytecodeProcessor* vm) {
    using namespace boost::json;
    return serialize(value_from(value));
}

std::string Impl::httpRequest(std::string method, std::string host, std::string target, std::string body, BytecodeProcessor *vm){
    
    
    asio::ip::tcp::resolver res(vm->ownerUnit->ctx); 
    beast::tcp_stream stream(vm->ownerUnit->ctx); 
    int version = 11; 

    auto const results = res.resolve(host, "80"); 
    stream.connect(results); 

    http::request<http::string_body> req{
        method == "POST" ? http::verb::post : http::verb::get,  
        target, 
        version 
    }; 

    req.set(http::field::host, host); 
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING); 

    if(method == "POST"){
        req.set(http::field::content_type, "application/json");
        req.body() = body;
        req.prepare_payload();
    }

    http::write(stream, req);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);


    stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both);

    return response.body();


}

bool Impl::containsType(BlsType value, std::string typeName, BytecodeProcessor* vm) {
    auto type = getTypeFromName(typeName);

    switch (type) {
        case TYPE::void_t:
        case TYPE::COUNT:
            throw std::runtime_error("Invalid type name provided.");
        break;

        case TYPE::ANY:
            return true;
        break;

        case TYPE::bool_t:
            return std::holds_alternative<bool>(value);
        break;

        case TYPE::int_t:
            return std::holds_alternative<int64_t>(value);
        break;
        
        case TYPE::float_t:
            return std::holds_alternative<double>(value);
        break;

        case TYPE::string_t:
            return std::holds_alternative<std::string>(value);
        break;

        case TYPE::list_t:
            return std::holds_alternative<std::shared_ptr<HeapDescriptor>>(value)
                && std::dynamic_pointer_cast<VectorDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(value));
        break;

        case TYPE::map_t:
            return std::holds_alternative<std::shared_ptr<HeapDescriptor>>(value)
                && std::dynamic_pointer_cast<MapDescriptor>(std::get<std::shared_ptr<HeapDescriptor>>(value));
        break;
        
        default:
            return false; // for now ignore devtypes
        break;
    }
}