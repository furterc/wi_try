/*
 * rtc_alarm.h
 *
 *  Created on: 15 Oct 2018
 *      Author: christo
 */

#ifndef SRC_RTC_ALARM_H_
#define SRC_RTC_ALARM_H_

#include "mbed.h"

typedef struct{
    uint8_t hour;
    uint8_t minute;
}sRTCAlarmObj_t;

class RTC_Alarm
{
    uint8_t triggered;
    void (*triggerCallback)(uint8_t triggered);

    sRTCAlarmObj_t startTime;
    sRTCAlarmObj_t endTime;

public:
    RTC_Alarm();
    virtual ~RTC_Alarm();

    void setTiggerCallback(void (*cb)(uint8_t triggered));
    void setAlarm(sRTCAlarmObj_t *start, sRTCAlarmObj_t *end);
    void checkAlarm();
};

#endif /* SRC_RTC_ALARM_H_ */
