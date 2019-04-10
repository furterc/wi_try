/*
 * InlineController.cpp
 *
 *  Created on: 24 Mar 2019
 *      Author: christo
 */

#include <InlineController.h>

InlineController::InlineController(InlineFan *fan) : mFan(fan), stateChange(0)
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

void InlineController::setStateChangeCallback(void (*state_change)(eInlineFanSpeed speed))
{
    if(state_change)
        stateChange = state_change;
}

void InlineController::setThresholds(sTempThresholds_t *th)
{
    memcpy(&mThresholds, th, sizeof(sTempThresholds_t));
}

int InlineController::updateTemperature(int temp)
{
    if(mThresholds.inlineOverride == 1)
    {
        printf("inline -- overrided\n");
        return 1;
    }

    eInlineFanSpeed speed =  INLINE_OFF;

    if(temp > mThresholds.inlineHighOn)
    {
        speed =  INLINE_HIGH_SPEED;
        if(getSpeed() != speed)
        {
            if(stateChange)
                stateChange(speed);
        }
        return updateFanSpeed(speed);
    }

    if(temp > mThresholds.inlineLowOn)
    {
        if(getSpeed() == INLINE_HIGH_SPEED && temp > mThresholds.inlineHighOff)
            return 0;

        speed =  INLINE_LOW_SPEED;
        if(getSpeed() != speed)
        {
            if(stateChange)
                stateChange(speed);
        }
        return updateFanSpeed(speed);
    }

    if(temp > mThresholds.inlineHighOff)
    {
        speed =  INLINE_OFF;
        if(getSpeed() != speed)
        {
            if(stateChange)
                stateChange(speed);
        }
        return updateFanSpeed(speed);
    }

    return 0;
}

eInlineFanSpeed InlineController::getSpeed()
{
    return mFan->getSpeed();
}
