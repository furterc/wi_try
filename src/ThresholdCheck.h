/*
 * ThresholdCheck.h
 *
 *  Created on: 03 Apr 2019
 *      Author: christo
 */

#ifndef SRC_THRESHOLDCHECK_H_
#define SRC_THRESHOLDCHECK_H_

#include <mbed.h>
#include "nvm_config.h"

#define THRESHOLD_TEMPERATURE   1
#define THRESHOLD_HUMIDITY      2

#define THRESHOLD_STATE_CLEAR   0
#define THRESHOLD_STATE_ALARM   1

class ThresholdCheck
{
    sTempThresholds_t mThresholds;
    void (*mAlarmCallback)(uint8_t state, uint8_t th, int limit, int value);

    uint8_t mAlarmTemperature;
    uint8_t mAlarmHumidity;
public:
    ThresholdCheck();
    virtual ~ThresholdCheck();

    void setAlarmCallback(void (*alarm_callback)(uint8_t state, uint8_t th, int limit, int value));
    void setThresholds(sTempThresholds_t *th);

    void getAlarms(uint8_t &t, uint8_t &h);
    void run(int temp, int humid);
};

#endif /* SRC_THRESHOLDCHECK_H_ */
