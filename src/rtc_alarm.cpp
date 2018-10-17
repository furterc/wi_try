/*
 * rtc_alarm.cpp
 *
 *  Created on: 15 Oct 2018
 *      Author: christo
 */

#include <rtc_alarm.h>
#include <string.h>

RTC_Alarm::RTC_Alarm()
{
    triggered = false;
    triggerCallback = 0;
}

RTC_Alarm::~RTC_Alarm()
{
    // TODO Auto-generated destructor stub
}

void RTC_Alarm::setTiggerCallback(void (*cb)(uint8_t triggered))
{
    triggerCallback = cb;
}

void RTC_Alarm::setAlarm(sRTCAlarmObj_t *start, sRTCAlarmObj_t *end)
{
    memcpy(&startTime, start, sizeof(sRTCAlarmObj_t));
    memcpy(&endTime, end, sizeof(sRTCAlarmObj_t));

    printf("  - startTime: %02d:%02d\n", startTime.hour, startTime.minute);
    printf("  -   endTime: %02d:%02d\n", endTime.hour, endTime.minute);
}

void RTC_Alarm::checkAlarm()
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    int hour = timeinfo->tm_hour;
    int min = timeinfo->tm_min;

    if(!triggered)
    {
        if ((hour != startTime.hour) || (min != startTime.minute))
            return;

        triggered = 1;
    }
    else
    {
        if ((hour != endTime.hour) || (min != endTime.minute))
            return;

        triggered = 0;
    }

    if(triggerCallback)
        triggerCallback(triggered);

    printf("Alarm Triggered %d @ %02d:%02d!\n", triggered, hour,min);

//    printf ( "Current local time and date: %s", asctime (timeinfo) );


//
//    struct tm t;
//    time(mktime(&t));

}
