/*
 * PCF8574.h
 *
 *  Created on: 20 Oct 2018
 *      Author: christo
 */

#ifndef IICDISPLAY_PCF8574_H_
#define IICDISPLAY_PCF8574_H_

#include "mbed.h"

#define PCF8574_ADDRBASE (0x27) //device base address

#define PCF8574_I2CINIT 1 //init i2c

#define PCF8574_MAXPINS 8 //max pin per device

class PCF8574
{
    I2C *mI2C;

    uint8_t mDeviceAddress;
    uint8_t pinStatus;

public:
    PCF8574(I2C *i2c, uint8_t deviceAddress);
    virtual ~PCF8574();


    uint8_t getoutput();
    int getOutputPin(uint8_t pin);
    int setOutput(uint8_t data);
    int setOutputPinHigh(uint8_t pin);
    int setOutputPinLow(uint8_t pin);

    int setOutputPins(uint8_t pinStart, uint8_t pinLength, uint8_t data);
    int setOutputPin(uint8_t pin, uint8_t data);

    int getInput();
    int getInputPin(uint8_t pin);
};

#endif /* IICDISPLAY_PCF8574_H_ */
