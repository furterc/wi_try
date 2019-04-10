/*
 * ThresholdCheck.cpp
 *
 *  Created on: 03 Apr 2019
 *      Author: christo
 */

#include <ThresholdCheck.h>

ThresholdCheck::ThresholdCheck() : mAlarmCallback(0), mAlarmTemperature(0), mAlarmHumidity(0)
{

}

ThresholdCheck::~ThresholdCheck()
{

}

void ThresholdCheck::setAlarmCallback(void (*alarm_callback)(uint8_t state, uint8_t th, int limit, int value))
{
    printf("alarm callback: %p\n", alarm_callback);
    if(alarm_callback)
        mAlarmCallback = alarm_callback;
}

void ThresholdCheck::setThresholds(sTempThresholds_t *th)
{
    memcpy(&mThresholds, th, sizeof(sTempThresholds_t));
}

void ThresholdCheck::getAlarms(uint8_t &t, uint8_t &h)
{
    t = mAlarmTemperature;
    h = mAlarmHumidity;
}

void ThresholdCheck::run(int temp, int humid)
{
    if(temp > mThresholds.alarmTemperature)
    {
        if(!mAlarmTemperature)
        {
            mAlarmTemperature = 1;
            printf("alarm\n");
            if(mAlarmCallback)
                mAlarmCallback(THRESHOLD_STATE_ALARM, THRESHOLD_TEMPERATURE, mThresholds.alarmTemperature, temp);
        }
    }
    else
    {
        if(mAlarmTemperature)
        {
            mAlarmTemperature = 0;
            if(mAlarmCallback)
                mAlarmCallback(THRESHOLD_STATE_CLEAR, THRESHOLD_TEMPERATURE, mThresholds.alarmTemperature, temp);
        }
    }

    if(humid > mThresholds.alarmHumidity)
    {
        if(!mAlarmHumidity)
        {
            mAlarmHumidity = 1;
            if(mAlarmCallback)
                mAlarmCallback(THRESHOLD_STATE_ALARM, THRESHOLD_HUMIDITY, mThresholds.alarmHumidity, humid);
        }
    }
    else
    {
        if(mAlarmHumidity)
        {
            mAlarmHumidity = 0;
            if(mAlarmCallback)
                mAlarmCallback(THRESHOLD_STATE_CLEAR, THRESHOLD_HUMIDITY, mThresholds.alarmHumidity, humid);
        }
    }
}
