/*
 * InlineController.h
 *
 *  Created on: 24 Mar 2019
 *      Author: christo
 */

#ifndef SRC_INLINECONTROLLER_H_
#define SRC_INLINECONTROLLER_H_

#include "mbed.h"
#include "nvm_config.h"
#include "InlineFan.h"

class InlineController
{
    InlineFan * mFan;
    sTempThresholds_t mThresholds;

    int updateFanSpeed(eInlineFanSpeed speed);
    void (*stateChange)(eInlineFanSpeed speed);

public:
    InlineController(InlineFan *fan);
    virtual ~InlineController();

    void setStateChangeCallback(void (*stateChange)(eInlineFanSpeed speed));
    void setThresholds(sTempThresholds_t *th);

    int updateTemperature(int temp);
    eInlineFanSpeed getSpeed();
};

#endif /* SRC_INLINECONTROLLER_H_ */
