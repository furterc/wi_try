/*
 * Relay.h
 *
 *  Created on: 17 Mar 2019
 *      Author: christo
 */

#ifndef SRC_RELAY_H_
#define SRC_RELAY_H_

#include "mbed.h"

class Relay : DigitalInOut
{
public:
    Relay(PinName pin);
    virtual ~Relay();

    void latch(int state);
    int get();
};

#endif /* SRC_RELAY_H_ */
