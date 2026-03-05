#ifdef __RPI64__

/**********************************************************************
* Filename    : DHT.hpp
* Description : DHT Temperature & Humidity Sensor library for Raspberry.
                Used for Raspberry Pi.
*				Program transplantation by Freenove.
* Author      : freenove
* modification: 2020/10/16
* Reference   : https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTlib
**********************************************************************/
#include "include/DHT11/DHT.hpp"
#include <pigpio.h>
#include <stdexcept>


void DHT::init(){
    if(gpioInitialise() < 0){
		throw std::runtime_error("Could not find gpio device!");
	};
}
//Function: Read DHT sensor, store the original data in bits[]
// return values:DHTLIB_OK   DHTLIB_ERROR_CHECKSUM  DHTLIB_ERROR_TIMEOUT
int DHT::readSensor(int pin, int wakeupDelay) {
        int mask = 0x80;
        int idx = 0;

        for (int i = 0; i < 5; i++) {
            bits[i] = 0;
        }

        // Send start signal
        gpioSetMode(pin, PI_OUTPUT);
        gpioWrite(pin, PI_HIGH);
        gpioDelay(500 * 1000);  // 500 ms

        gpioWrite(pin, PI_LOW);
        gpioDelay(wakeupDelay * 1000);  // Wakeup delay in µs

        gpioWrite(pin, PI_HIGH);
        gpioDelay(40);  // 40 µs

        gpioSetMode(pin, PI_INPUT);

        int32_t t;
        int32_t loopCnt;

        // Wait for sensor to pull pin LOW
        loopCnt = DHTLIB_TIMEOUT;
        t = gpioTick();
        while (gpioRead(pin) == PI_HIGH) {
            if ((gpioTick() - t) > loopCnt)
                return DHTLIB_ERROR_TIMEOUT;
        }

        // Wait for sensor to pull pin HIGH
        loopCnt = DHTLIB_TIMEOUT;
        t = gpioTick();
        while (gpioRead(pin) == PI_LOW) {
            if ((gpioTick() - t) > loopCnt)
                return DHTLIB_ERROR_TIMEOUT;
        }

        // Wait for sensor to pull pin LOW again
        loopCnt = DHTLIB_TIMEOUT;
        t = gpioTick();
        while (gpioRead(pin) == PI_HIGH) {
            if ((gpioTick() - t) > loopCnt)
                return DHTLIB_ERROR_TIMEOUT;
        }

        // Read 40 bits (5 bytes)
        for (int i = 0; i < 40; i++) {
            loopCnt = DHTLIB_TIMEOUT;

            // Wait for LOW
            t = gpioTick();
            while (gpioRead(pin) == PI_LOW) {
                if ((gpioTick() - t) > loopCnt)
                    return DHTLIB_ERROR_TIMEOUT;
            }

            // Measure length of HIGH pulse
            t = gpioTick();
            loopCnt = DHTLIB_TIMEOUT;
            while (gpioRead(pin) == PI_HIGH) {
                if ((gpioTick() - t) > loopCnt)
                    return DHTLIB_ERROR_TIMEOUT;
            }

            if ((gpioTick() - t) > 60) {
                bits[idx] |= mask;
            }

            mask >>= 1;
            if (mask == 0) {
                mask = 0x80;
                idx++;
            }
        }

        gpioSetMode(pin, PI_OUTPUT);
        gpioWrite(pin, PI_HIGH);

        return DHTLIB_OK;
    }

//Function：Read DHT sensor, analyze the data of temperature and humidity
//return：DHTLIB_OK   DHTLIB_ERROR_CHECKSUM  DHTLIB_ERROR_TIMEOUT
int DHT::readDHT11Once(int pin) {
        int rv;
        uint8_t sum;

        rv = readSensor(pin, DHTLIB_DHT11_WAKEUP);
        if (rv != DHTLIB_OK) {
            humidity = DHTLIB_INVALID_VALUE;
            temperature = DHTLIB_INVALID_VALUE;
            return rv;
        }

        humidity = bits[0];
        temperature = bits[2] + bits[3] * 0.1;
        sum = bits[0] + bits[1] + bits[2] + bits[3];

        if (bits[4] != sum)
            return DHTLIB_ERROR_CHECKSUM;

        return DHTLIB_OK;
}

int DHT::readDHT11(int pin) {
	int chk = DHTLIB_INVALID_VALUE;
	for (int i = 0; i < 15; i++) {
		chk = readDHT11Once(pin);
		if (chk == DHTLIB_OK) {
			return DHTLIB_OK;
		}
		gpioDelay(100 * 1000); // 100 ms
	}
	return chk;
}

#endif



