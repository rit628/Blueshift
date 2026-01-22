#pragma once

#ifdef __RPI64__
#include <pigpio.h>
#endif

#include <cstdint>
#include <iostream>


/* 
    Currently blueshift only supports the ADS7830 as an 
    ADC adaptor. More ADS will come soon with a virtual class
*/

class ADS7830{
    private: 
        int handle;
        static constexpr uint8_t channels[8] = {  0x84, 0xC4, 0x94, 0xD4, 0xA4, 0xE4, 0xB4, 0xF4};
    
    public:
        ADS7830(){
            this->handle = -1;
            #ifdef __RPI64__
            if(gpioInitialise() < 0){
                std::cout<<"Failed to initialse GPIO for ADC detection"<<std::endl;
            }

            this->handle = i2cOpen(1, 0x4B, 0);
            if(this->handle < 0){
                std::cerr<<"Could not find ADC device on RPI 64!"<<std::endl;
            }
            #endif
        }

        int readByte(int fromChannel){
            #ifdef __RPI64__
            if(this->handle  != -1){
                i2cWriteByte(this->handle, channels[fromChannel]);
                gpioDelay(1000);
                return i2cReadByte(this->handle);
            }
            #endif
            return 0;
        } 

        void close(){
            if (!isValid()) return;
            #ifdef __RPI64__
            i2cClose(this->handle);
            #endif
        }

        bool isValid(){
            return this->handle >= 0;
        }
}; 
