/*
 * InlineController.cpp
 *
 *  Created on: 24 Mar 2019
 *      Author: christo
 */

#include <InlineController.h>

InlineController::InlineController(InlineFan *fan) : mFan(fan)
{

}

InlineController::~InlineController()
{

}

int InlineController::updateFanSpeed(eInlineFanSpeed speed)
{
    if(speed != getSpeed())
    {
        mFan->setSpeed(speed);
        return 1;
    }

    return 0;
}

void InlineController::setThresholds(sTempThresholds_t *th)
{
    memcpy(&mThresholds, th, sizeof(sTempThresholds_t));
}

int InlineController::updateTemperature(int temp)
{
    if(temp > mThresholds.inlineHighOn)
        return updateFanSpeed(INLINE_HIGH_SPEED);

    if(temp > mThresholds.inlineLowOn)
    {
        if(getSpeed() == INLINE_HIGH_SPEED && temp > mThresholds.inlineHighOff)
            return 0;

        return updateFanSpeed(INLINE_LOW_SPEED);
    }

    if(temp > mThresholds.inlineHighOff)
        return updateFanSpeed(INLINE_OFF);

    return 0;
}

eInlineFanSpeed InlineController::getSpeed()
{
    return mFan->getSpeed();
}
