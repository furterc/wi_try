/*
 * Relay.h
 *
 *  Created on: 17 Mar 2019
 *      Author: christo
 */

#ifndef SRC_RELAY_H_
#define SRC_RELAY_H_

#include "mbed.h"

enum eRelayState{
    RELAY_NC = 0,
    RELAY_NO = 1
};

class Relay : DigitalInOut
{
    eRelayState mState;

public:
    Relay(PinName pin, eRelayState state = RELAY_NO);
    virtual ~Relay();

    void latch(int state);
    int get();
};

#endif /* SRC_RELAY_H_ */
