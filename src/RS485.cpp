/*
 * RS485.cpp
 *
 *  Created on: 10 Nov 2018
 *      Author: christo
 */

#include <RS485.h>
#include "caboodle/utils.h"

RS485* RS485::__instance = 0;

RS485::RS485(UARTSerial *serial, DigitalOut *writeEnable) : mSerial(serial),
                                                            mWriteEnable(writeEnable),
                                                            listenThread(osPriorityLow, OS_STACK_SIZE, NULL, "RS485")
{
    listenThread.start(callback(ReceiveSerial, this));
    framer = new cHDLCframer(64);
    writeEnable->write(0);
    cb = 0;
}

RS485::~RS485()
{
    if(framer)
        delete(framer);
}

void RS485::init(UARTSerial *serial, DigitalOut *writeEnable)
{
    if(__instance == 0)
        __instance = new RS485(serial, writeEnable);
}

void RS485::ReceiveSerial(RS485 *instance)
{
    printf("RS485: 0x%X\n", (int)Thread::gettid());

    while(__instance)
    {
        uint8_t data = 0;
        if(__instance->mSerial->read(&data, 1) == 1)
            __instance->handleByte(data);

        wait_us(10);
//        __instance->handleByte(__instance->mSerial->getc());
    }
}

void RS485::handleByte(uint8_t byte)
{
    int len = framer->pack(byte);

    if(len > 0)
    {
        printf("frame in ");
        uint8_t data[64];
        memcpy(data, framer->buffer(), len);
        diag_dump_buf(data, len);
        if(__instance->cb)
            __instance-> cb(data, (uint8_t)len);
    }
    if(len == -1)
    {
        printf(RED("frame err\n"));
    }
}

void RS485::send(uint8_t *data, int len)
{
    if(len > 64)
        return;

    uint8_t framedData[128];
    uint32_t frameLen = 0;
    __instance->framer->frame(data, len, framedData, &frameLen);

    __instance->mWriteEnable->write(1);

    __instance->mSerial->write(framedData, frameLen);
    wait_us(1000);

    __instance->mWriteEnable->write(0);

    printf("485send: ");
    diag_dump_buf(framedData, frameLen);
}

void RS485::setReceiveCallback(void (*callback)(uint8_t *data, uint8_t len))
{
    if(callback)
        __instance->cb = callback;
}



