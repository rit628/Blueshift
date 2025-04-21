#include "Devices.hpp"
#include "libDM/DynamicMessage.hpp"
//#include <pigpio.h>

using namespace Device;

/* TIMER_TEST */

void TIMER_TEST::proc_message_impl(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);
    write_file << std::to_string(this->states.test_val) << ",";  
    write_file.flush(); 
}

void TIMER_TEST::set_ports(std::unordered_map<std::string, std::string> &srcs) {
    filename = "./samples/client/" + srcs["file"]; 
    write_file.open(filename);
    if(write_file.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    } 
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }
}

void TIMER_TEST::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

TIMER_TEST::~TIMER_TEST() {
    write_file.close();
}

/* LINE_WRITER */

void LINE_WRITER::proc_message_impl(DynamicMessage& dmsg) {
    dmsg.unpackStates(states);

    if (this->file_stream.is_open()) {
        this->file_stream.close();  // Close the file before reopening
    }
    
    this->file_stream.open(this->filename, std::ios::out | std::ios::trunc);  // Open in truncate mode
    
    if (this->file_stream.is_open()) {
        std::cout<<"Writing data"<<std::endl; 
        this->file_stream << states.msg; 
        this->file_stream.flush();  // Ensure data is written immediately
        this->file_stream.close();
    }
    else {
        std::cout << "file didnt open" << std::endl;
    }
}

// can add better keybinding libraries in the future
bool LINE_WRITER::handleInterrupt(){

    this->file_stream.open(this->filename, std::ios::in);
    this->file_stream.clear(); 
    this->file_stream.seekg(0, std::ios::beg); 

    std::string line; 

    auto getL = bool(std::getline(this->file_stream, line));

    if(getL){
        this->states.msg = line;
        return true; 
    }
    else {
        std::cout << "unc status" << std::endl;
    }
    
    return false; 
}

void LINE_WRITER::set_ports(std::unordered_map<std::string, std::string> &src) {
    this->filename = "./samples/client/" + src["file"]; 
    
    this->file_stream.open(filename, std::ios::in | std::ios::out); 
    if(file_stream.is_open()){
        std::cout<<"Could find file"<<std::endl; 
    }
    else{
        std::cout<<"Could not find file"<<std::endl; 
    }

    // Add the interrupt and handler
    this->addFileIWatch(this->filename, [this](){return this->handleInterrupt();}); 
}

void LINE_WRITER::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

LINE_WRITER::~LINE_WRITER() {
    file_stream.close();
}

/* LIGHT */

void LIGHT::proc_message_impl(DynamicMessage &dmsg) {
    dmsg.unpackStates(states);
    if (states.state) {
        gpioWrite(this->PIN, PI_ON);
    }
    else {
        gpioWrite(this->PIN, PI_OFF);
    }
}

void LIGHT::set_ports(std::unordered_map<std::string, std::string> &src) {
    this->PIN = std::stoi(src["PIN"]);
    if (gpioInitialise() == PI_INIT_FAILED) {
            std::cerr << "GPIO setup failed" << std::endl;
            return;
    }
    gpioSetMode(this->PIN, PI_OUTPUT);
}

void LIGHT::read_data(DynamicMessage &dmsg) {
    dmsg.packStates(states);
}

LIGHT::~LIGHT() {
    gpioTerminate();
}

/* BUTTON */
bool BUTTON::handleInterrupt()
{
    // 3) ISR context: just set your flag
    pressed.store(true, std::memory_order_relaxed);
    // return true to keep watching; return false to auto‐unregister
    return true;
}

void BUTTON::set_ports(std::unordered_map<std::string, std::string> &src)
{
    this->PIN = std::stoi(src.at("PIN"));
    if (gpioInitialise() == PI_INIT_FAILED) 
    {
        std::cerr << "GPIO setup failed" << std::endl;
        return;
    }
    gpioSetMode(this->PIN, PI_INPUT);
    gpioSetPullUpDown(this->PIN, PI_PUD_UP);
    //IGpioWatcher(this->PIN, [this](){return this->handleInterrupt();});
}

void BUTTON::proc_message_impl(DynamicMessage &dmsg)
{
    dmsg.unpackStates(states);
}

void BUTTON::read_data(DynamicMessage &dmsg)
{
    states.pressed = pressed.exchange(false, std::memory_order_relaxed);
    //states.pressed = false;
    dmsg.packStates(states);
}

BUTTON::~BUTTON() 
{
    gpioTerminate();
}

/* SWITCH */
void SWITCH::set_ports(std::unordered_map<std::string, std::string> &src)
{
    this->PIN = std::stoi(src.at("PIN"));
    std::string pull = src.count("PULL") ? src.at("PULL") : "DOWN";

    if(gpioInitialise() == PI_INIT_FAILED) 
    {
        std::cerr << "GPIO init failed" << std::endl;           
        return;
    }
    gpioSetMode(this->PIN, PI_INPUT);
    if(pull == "UP") 
    {
        gpioSetPullUpDown(this->PIN, PI_PUD_UP);
    } 
    else 
    {
        gpioSetPullUpDown(this->PIN, PI_PUD_DOWN);        
    }
}

void SWITCH::proc_message_impl(DynamicMessage &dmsg)
{
    dmsg.unpackStates(states);
}

void SWITCH::read_data(DynamicMessage &dmsg)
{
    // If pulled‐down: HIGH = closed, LOW = open
    // If pulled‐up:   LOW  = closed, HIGH = open
    bool closed = (gpioRead(this->PIN) == (up ? PI_LOW : PI_HIGH));
    states.closed = closed;
    dmsg.packStates(states);
}

SWITCH::~SWITCH() 
{
    gpioTerminate();
}

/* MOTOR */

void MOTOR::set_ports(std::unordered_map<std::string, std::string> &src)
{
    in1_pin  = std::stoi(src.at("IN1"));
    in2_pin = std::stoi(src.at("IN2"));
    pwm_pin = std::stoi(src.at("PWM"));
    pwm_range = std::stoi(src["PWM_RANGE"]);
    if(gpioInitialise() == PI_INIT_FAILED) 
    {
        std::cerr << "GPIO setup failed" << std::endl;
        return;
    }

    // Direction pins are outputs
    gpioSetMode(in1_pin, PI_OUTPUT);
    gpioSetMode(in2_pin, PI_OUTPUT);

    // PWM pin
    gpioSetMode(pwm_pin, PI_OUTPUT);
    gpioSetPWMrange(pwm_pin, pwm_range);
    // Optional: set a default PWM frequency (Hz)
    gpioSetPWMfrequency(pwm_pin, 1000);
}

void MOTOR::proc_message_impl(DynamicMessage &dmsg)
{
    dmsg.unpackStates(states);

}

void MOTOR::read_data(DynamicMessage &dmsg)
{
    dmsg.packStates(states);
    apply_speed(states.speed);
}

void MOTOR::apply_speed(int speed)
{
    if(speed > 100) speed = 100;
        if(speed < -100) speed = -100;

        if(speed == 0) 
        {
            // brake / both low
            gpioWrite(in1_pin, PI_OFF);
            gpioWrite(in2_pin, PI_OFF);
            gpioPWM(pwm_pin, 0);
        }
        else if(speed > 0) 
        {
            // forward
            gpioWrite(in1_pin, PI_ON);
            gpioWrite(in2_pin, PI_OFF);
            // scale 0–100 to 0–pwm_range
            int duty = (speed * pwm_range) / 100;
            gpioPWM(pwm_pin, duty);
        }
        else 
        {
            // backward
            gpioWrite(in1_pin, PI_OFF);
            gpioWrite(in2_pin, PI_ON);
            int duty = ((-speed) * pwm_range) / 100;
            gpioPWM(pwm_pin, duty);
        }
}

MOTOR::~MOTOR() 
{
    gpioTerminate();
}


/* POTENTIOMETER */
void POTENTIOMETER::set_ports(std::unordered_map<std::string, std::string> &src)
{
    spi_channel = std::stoi(src.at("SPI_CHANNEL"));
    spi_speed = std::stoi(src.at("SPI_SPEED"));
    adc_channel = std::stoi(src.at("ADC_CHANNEL"));
        
    if(gpioInitialise() == PI_INIT_FAILED) {
        std::cerr << "GPIO init failed" << std::endl;
        return;
    }
    // Open SPI channel; flags=0 selects mode 0, CS uses CE0/CE1 by channel
    spi_handle = spiOpen(spi_channel, spi_speed, 0);
    if(spi_handle < 0) 
    {
        std::cerr << "Failed to open SPI channel: " << spi_handle << std::endl;
    }
}

void POTENTIOMETER::proc_message_impl(DynamicMessage &dmsg)
{
    dmsg.unpackStates(states);
}

void POTENTIOMETER::read_data(DynamicMessage &dmsg)
{
    if(spi_handle < 0) 
    {
        std::cerr << "SPI not initialized" << std::endl;
        return;
    }

    // MCP3008 protocol: start bit, single‑ended, channel bits
    char tx[3];
    tx[0] = 0x01;                                      // start bit
    tx[1] = static_cast<char>(0x80 | (adc_channel << 4));
    tx[2] = 0x00;

    char rx[3] = {0};
    int status = spiXfer(spi_handle, tx, rx, 3);
    if (status < 0) 
    {
        std::cerr << "SPI transfer failed: " << status << std::endl;
        return;
    }

    // assemble 10‑bit result: lower 2 bits of rx[1], all of rx[2]
    int raw = ((rx[1] & 0x03) << 8) | rx[2];
    states.raw = raw;
    states.normalized = raw / 1023.0;

    dmsg.packStates(states);
}

POTENTIOMETER::~POTENTIOMETER() 
{               
    if (spi_handle >= 0)
    {
        spiClose(spi_handle);
    }
    gpioTerminate(); 
}


std::shared_ptr<AbstractDevice> getDevice(DEVTYPE dtype, std::unordered_map<std::string, std::string> &port_nums, int device_alias) {
    switch(dtype){
        #define DEVTYPE_BEGIN(name) \
        case DEVTYPE::name: { \
            auto devPtr = std::make_shared<name>(); \
            devPtr->set_ports(port_nums); \
            return devPtr; \
            break; \
        }
        #define ATTRIBUTE(...)
        #define DEVTYPE_END
        #include "DEVTYPES.LIST"
        #undef DEVTYPE_BEGIN
        #undef ATTRIBUTE
        #undef DEVTYPE_END
        default : {
            throw std::invalid_argument("Unknown dtype accessed!"); 
        }
    }; 
}; 