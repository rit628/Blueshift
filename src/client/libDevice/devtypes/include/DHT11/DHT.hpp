/**********************************************************************
* Filename    : DHT.hpp
* Description : DHT Temperature & Humidity Sensor library for Raspberry.
                Used for Raspberry Pi.
*               Program transplantation by Freenove.
* Author      : freenove
* modification: 2020/10/16
* Reference   : https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
**********************************************************************/
#include <stdint.h>

class DHT{      
    public:
        ////read return flag of sensor
        static constexpr int DHTLIB_OK =  0;
        static constexpr int DHTLIB_ERROR_CHECKSUM = -1;
        static constexpr int DHTLIB_ERROR_TIMEOUT = -2;
        static constexpr int DHTLIB_INVALID_VALUE = -999;

        static constexpr int DHTLIB_DHT11_WAKEUP =  20;
        static constexpr int DHTLIB_DHT_WAKEUP =  1;

        static constexpr int DHTLIB_TIMEOUT =  100;

        double humidity,temperature;    //use to store temperature and humidity data read
        void init();
        int readDHT11Once(int pin);     //read DHT11
        int readDHT11(int pin);     //read DHT11
    private:
        uint8_t bits[5];    //Buffer to receiver data
        int readSensor(int pin,int wakeupDelay);    //
        
};
