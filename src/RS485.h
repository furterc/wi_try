/*
 * RS485.h
 *
 *  Created on: 10 Nov 2018
 *      Author: christo
 */

#ifndef SRC_RS485_H_
#define SRC_RS485_H_

#include "mbed.h"
#include "mbed-os/rtos/Thread.h"
#include <Serial.h>
#include "caboodle/hdlc_framer.h"

class RS485
{
    static RS485 *__instance;
    RS485(UARTSerial *serial, DigitalOut *writeEnable);

    UARTSerial *mSerial;
    DigitalOut *mWriteEnable;
    rtos::Thread listenThread;

    uint8_t mHead;
    uint8_t mTail;
    uint8_t rxBuffer[64];

    static void ReceiveSerial(RS485 *instance);

    void handleByte(uint8_t byte);

    void (*cb)(uint8_t *data, int len);

    cHDLCframer *framer;

public:
    static void init(UARTSerial *serial, DigitalOut *writeEnable);
    static void send(uint8_t *data, int len);
    static void setReceiveCallback(void (*callback)(uint8_t *data, int len));
    virtual ~RS485();
};

#endif /* SRC_RS485_H_ */
