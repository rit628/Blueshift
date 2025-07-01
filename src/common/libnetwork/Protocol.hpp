#pragma once

#include <stdint.h>
#include <vector> 
#include "include/Common.hpp"

#define MAX_NAME_LEN 128

using TimerID = uint16_t;

// Can hold a totoal of about 65,000 device types
enum class DTYPE : uint16_t{
    LED, 
    MOTOR, 
    HUMID, 
    BUTTON, 
    MOISTURE, 
}; 


enum class Protocol : uint8_t{
    // Configuration and Handshake
    CONFIG_INFO, 
    CONFIG_OK, 

    // (Master -> Client)
    STATE_CHANGE, 
    TICKER_UPDATE, 
    BEGIN, 
    SHUTDOWN, 
    TICKER_INITIAL, 
    MASTER_ERROR, 

    // (Client -> Master)
    CONFIG_NAME, 
    SEND_STATE_INIT, 
    SEND_STATE, 
    CALLBACK, 
    CLIENT_ERROR, 

    // Client Loop back
    CONNECTION_LOST 
}; 

enum class ERROR_T{
    BAD_DEV_CONFIG, 
    INCOMPATIBLE_VERSIONING, 
    MASTER_DISCONNECT, 
    FATAL_ERROR,
    CLIENT_DISCONNECT, 
    DEVICE_FAILURE, 
}; 



/*
    HANDSHAKE CONFIGURATION MESSAGE: 
    This should contain all the info for a specific message
*/

struct DeviceConfigMsg{
    // Device type
    std::vector<TYPE> type; 

    // Numeric alias for device
    std::vector<uint16_t> device_alias; 

    // Object
    std::vector<std::unordered_map<std::string, std::string>> srcs; 

    // Determines if the client should send and initial state
    std::vector<uint16_t> triggers;  

}; 


/*
    GENERAL MESSAGE PROTOCOL
*/

// Header object
struct SentHeader{
    Protocol prot = Protocol::SEND_STATE;
    uint16_t ctl_code = 0; 
    uint8_t device_code = 0; 
    uint32_t body_size = 0; 
    uint16_t timer_id = 0; 
    ERROR_T ec = ERROR_T::BAD_DEV_CONFIG; 

    bool fromInterrupt = false; 

    // Set by client
    float volatility = 0; 
    
}; 

// Sending message this is what is actually sent over the internet
struct SentMessage{
    SentHeader header; 
    std::vector<char> body; 
}; 


// Timer for a specific device: 
struct Timer{
    // Timer id to uniquely identify timers to determine what oblock to fwd stuff to
    TimerID id = 0; 
    // Device name
    uint16_t device_num = 0; 
    // Is a constant polling rate;
    bool const_poll = true; 
    // Polling period (ms)
    uint32_t period = 1000;
}; 









