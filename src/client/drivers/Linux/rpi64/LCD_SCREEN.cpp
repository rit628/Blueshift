
#ifdef __RPI64__
#include "include/LCD_SCREEN.hpp"
#include "libDM/DynamicMessage.hpp"
#include <pigpio.h>
#include <unordered_map>
#include <stdexcept>

#define MAX_BUF_SIZE 16

using namespace Device;


void LCD_SCREEN::pcf8574Write(uint8_t data) {
    i2cWriteByte(this->handle, data);
}

void LCD_SCREEN::lcdStrobe(uint8_t data) {
    pcf8574Write(data | EN | BL);
    gpioDelay(500);  // 500us
    pcf8574Write((data & ~EN) | BL);
    gpioDelay(500);  // 500us
}

void LCD_SCREEN::lcdWrite4Bits(uint8_t data) {
    pcf8574Write(data | BL);
    lcdStrobe(data);
}

void LCD_SCREEN::lcdSend(uint8_t value, uint8_t mode) {
    uint8_t high = value & 0xF0;
    uint8_t low  = (value << 4) & 0xF0;

    lcdWrite4Bits((high >> 4) << 4 | mode);
    lcdWrite4Bits((low >> 4) << 4 | mode);
}

void LCD_SCREEN::lcdCommand(uint8_t cmd) {
    lcdSend(cmd, 0);
}

void LCD_SCREEN::lcdChar(char c) {
    lcdSend(c, RS);
}

void LCD_SCREEN::lcdInit() {
    gpioDelay(50000);  // 50ms
    lcdWrite4Bits(0x30);
    gpioDelay(4500);
    lcdWrite4Bits(0x30);
    gpioDelay(4500);
    lcdWrite4Bits(0x30);
    gpioDelay(150);
    lcdWrite4Bits(0x20);  // 4-bit mode

    lcdCommand(0x28); // function set: 4-bit, 2 line, 5x8 dots
    lcdCommand(0x0C); // display on, cursor off, blink off
    lcdCommand(0x06); // entry mode
    lcdCommand(0x01); // clear display
    gpioDelay(2000);
}

void LCD_SCREEN::lcdPrint(const char *str) {
    while(*str) {
        lcdChar(*str++);
    }
}

void LCD_SCREEN::lcdSetCursor(int col, int row) {
    static uint8_t row_offsets[] = {0x00, 0x40};
    lcdCommand(0x80 | (col + row_offsets[row]));
}

void LCD_SCREEN::init(std::unordered_map<std::string, std::string> &config){
    // No arguments

    // Sets the actuator
    this->isActuator = true;

    if(gpioInitialise() < 0){
        std::cout<<"PI GPIO initialization failed!"<<std::endl;
        throw std::invalid_argument("PIGPIO init failed!");
    }

    this->handle = i2cOpen(1, PCF8574_ADDR, 0);
    if(handle < 0){
        std::cout<<"Could not open I2C for LCD"<<std::endl;
        gpioTerminate();
        throw std::invalid_argument( "Could not open I2C for LCD");
    }



    lcdInit();
    
}

void LCD_SCREEN::processStates(DynamicMessage &dmsg){
    dmsg.unpackStates(this->states);
    // 16 character messages max?
    char buf[MAX_BUF_SIZE];
    strncpy(buf, this->states.msg.c_str(), sizeof(buf)); 
    std::cout<<"THE MESSAGE IS: "<<this->states.msg<<std::endl;
    std::cout<<"THE COPIES MESSAGE IS: "<<buf<<std::endl;

    lcdCommand(0x01);
    gpioDelay(2000);
    this->lcdSetCursor(this->states.col, this->states.row);
    this->lcdPrint(buf); 
}

void LCD_SCREEN::transmitStates(DynamicMessage &dmsg){
    dmsg.packStates(this->states);
}




#endif