/*
 * InlineFan.h
 *
 *  Created on: 18 Mar 2019
 *      Author: christo
 */

#ifndef SRC_INLINEFAN_H_
#define SRC_INLINEFAN_H_

#include "mbed.h"
#include "Relay.h"

enum eInlineFanSpeed{
    INLINE_OFF,
    INLINE_LOW_SPEED,
    INLINE_HIGH_SPEED
};

class InlineFan
{
    Relay *mPowerRelay;
    Relay *mSpeedRelay;
    eInlineFanSpeed mFanSpeed;

public:
    InlineFan(Relay *powerRelay, Relay *speedRelay, eInlineFanSpeed fanSpeed = INLINE_LOW_SPEED);
    virtual ~InlineFan();

    void setSpeed(eInlineFanSpeed fanSpeed);
    eInlineFanSpeed getSpeed();
};

#endif /* SRC_INLINEFAN_H_ */
