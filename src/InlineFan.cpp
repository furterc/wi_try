/*
 * InlineFan.cpp
 *
 *  Created on: 18 Mar 2019
 *      Author: christo
 */

#include <InlineFan.h>

InlineFan::InlineFan(Relay *powerRelay, Relay *speedRelay, eInlineFanSpeed fanSpeed) : mPowerRelay(powerRelay), mSpeedRelay(speedRelay), mFanSpeed(fanSpeed)
{
    setSpeed(fanSpeed);
}

InlineFan::~InlineFan()
{

}

void InlineFan::setSpeed(eInlineFanSpeed fanSpeed)
{
    switch(fanSpeed)
    {
    case INLINE_OFF:
        mPowerRelay->latch(0);
        mSpeedRelay->latch(0);
        break;
    case INLINE_LOW_SPEED:
        mPowerRelay->latch(1);
        mSpeedRelay->latch(0);
        break;
    case INLINE_HIGH_SPEED:
        mPowerRelay->latch(1);
        mSpeedRelay->latch(1);
        break;
    }
    mFanSpeed = fanSpeed;
}

eInlineFanSpeed InlineFan::getSpeed()
{
    return mFanSpeed;
}
