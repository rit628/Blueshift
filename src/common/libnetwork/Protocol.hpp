#pragma once

#include <stdint.h>
#include <vector> 
#include "include/Common.hpp"

#define BROADCAST_PORT 2988
#define MAX_NAME_LEN 128
#define MASTER_PORT 60000

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

    // (Client -> Master)
    CONFIG_NAME, 
    SEND_STATE, 
    CONFIG_ERROR,
    CALLBACK, 

 
}; 

/*
    HANDSHAKE CONFIGURATION MESSAGE: 
    This should contain all the info for a specific message
*/

struct DeviceConfigMsg{

    // Device type
    std::vector<DEVTYPE> type; 

    // Numeric alias for device
    std::vector<uint16_t> device_alias; 

    // Object
    std::vector<std::unordered_map<std::string, std::string>> srcs; 

}; 


/*
    GENERAL MESSAGE PROTOCOL
*/

// Header object
struct SentHeader{
    Protocol prot;
    uint16_t ctl_code; 
    uint8_t device_code; 
    uint32_t body_size; 
    uint16_t timer_id; 
    bool fromInterrupt; 

    // Set by client
    float volatility; 
    
}; 

// Sending message this is what is actually sent over the internet
struct SentMessage{
    SentHeader header; 
    std::vector<char> body; 
}; 


// Timer for a specific device: 
struct Timer{
    // Timer id to uniquely identify timers to determine what oblock to fwd stuff to
    TimerID id; 
    // Device name
    uint16_t device_num; 
    // Is a constant polling rate;
    bool const_poll; 
    // Polling period (ms)
    uint32_t period;
}; 









