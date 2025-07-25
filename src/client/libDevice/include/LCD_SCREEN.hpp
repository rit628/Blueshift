#pragma once
#include "../DeviceCore.hpp"
#include "libtype/typedefs.hpp"

# define PCF8574_ADDR 0x27

#define RS 0x01
#define RW 0x02 
#define EN 0x04
#define BL 0x08

#define D4 0x10
#define D5 0x20
#define D6 0x40
#define D7 0x80

namespace Device{
    class LCD_SCREEN : public DeviceCore<TypeDef::LCD_SCREEN>{
        private:
            int handle;
            void pcf8574Write(uint8_t data);
            void lcdStrobe(uint8_t data);
            void lcdWrite4Bits(uint8_t data);
            void lcdSend(uint8_t value, uint8_t mode);
            void lcdCommand(uint8_t cmd);
            void lcdChar(char c);
            void lcdInit();
            void lcdPrint(const char *str);
            void lcdSetCursor(int col, int row);

        public:
            void processStates(DynamicMessage &dmsg);
            void init(std::unordered_map<std::string, std::string> &config);
            void transmitStates(DynamicMessage &dmsg);
            ~LCD_SCREEN();



    };
}