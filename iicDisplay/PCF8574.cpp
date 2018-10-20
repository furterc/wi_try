/*
 * PCF8574.cpp
 *
 *  Created on: 20 Oct 2018
 *      Author: christo
 */

#include <iicDisplay/PCF8574.h>
#include <string.h>

PCF8574::PCF8574(I2C *i2c, uint8_t deviceAddress) : mI2C(i2c), mDeviceAddress(deviceAddress), pinStatus(0)
{

}

PCF8574::~PCF8574()
{
    // TODO Auto-generated destructor stub
}

uint8_t PCF8574::getoutput()
{
    return pinStatus;
}

int PCF8574::getOutputPin(uint8_t pin)
{
    int data = -1;
    if(pin < PCF8574_MAXPINS)
    {
        data = pinStatus;
        data = (data >> pin) & 0b00000001;
    }
    return data;
}

int PCF8574::setOutput(uint8_t data)
{
    pinStatus = data;
    mI2C->write((PCF8574_ADDRBASE+mDeviceAddress)<<1, (char *)&data, 1, false);
    return 0;
}

int PCF8574::setOutputPins(uint8_t pinStart, uint8_t pinLength, uint8_t data)
{
    if((pinStart - pinLength + 1 >= 0 && pinStart - pinLength + 1 >= 0 && pinStart < PCF8574_MAXPINS && pinStart > 0 && pinLength > 0))
    {
        uint8_t b = pinStatus;
        uint8_t mask = ((1 << pinLength) - 1) << (pinStart - pinLength + 1);
        data <<= (pinStart - pinLength + 1);
        data &= mask;
        b &= ~(mask);
        b |= data;
        pinStatus = b;
        //update device
        mI2C->write((PCF8574_ADDRBASE+mDeviceAddress)<<1, (char *)&b, 1, false);

        return 0;
    }
    return -1;
}

int PCF8574::setOutputPin(uint8_t pin, uint8_t data)
{
    if(pin < PCF8574_MAXPINS)
    {
        uint8_t b = pinStatus;
        b = (data != 0) ? (b | (1 << pin)) : (b & ~(1 << pin));
        pinStatus = b;
        //update device
        mI2C->write((PCF8574_ADDRBASE+mDeviceAddress)<<1, (char *)&b, 1, false);
        return 0;
    }
    return -1;
}

int PCF8574::setOutputPinHigh(uint8_t pin)
{
    return setOutputPin(pin, 1);
}

int PCF8574::setOutputPinLow(uint8_t pin)
{
    return setOutputPin(pin, 0);
}

int PCF8574::getInput()
{
    int data = -1;
    mI2C->read((PCF8574_ADDRBASE+mDeviceAddress)<<1, (char*)&data, 1, false);
    return data;
}

int PCF8574::getInputPin(uint8_t pin)
{
    int data = -1;
    if(pin < PCF8574_MAXPINS)
    {
        data = getInput();
        if(data != -1)
        {
            data = (data >> pin) & 0b00000001;
        }
    }
    return data;
}

