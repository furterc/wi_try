/*
 * LCDController.h
 *
 *  Created on: 20 Nov 2018
 *      Author: christo
 */

#ifndef SRC_LCDCONTROLLER_H_
#define SRC_LCDCONTROLLER_H_


#include "LCDPCF8574.h"
#include "linked_list.h"

#define LCD_LOG_ENTRIES     16
#define LCD_LOG_TIMEOUT     5000

class LCDController
{
    sLinkedList_t mLinkedList;

    LCDPCF8574 *mLcd;

    uint32_t mNextTick;

    char *lcdLines[3];

public:
    LCDController(LCDPCF8574 *lcd);
    virtual ~LCDController();

    int logLine(char *data);
    void logUpdate();
    void logRun();
    void logUpdateDisplay();

    void clearLine(uint8_t line);
    void writeLine(char *data, uint8_t line);
};

#endif /* SRC_LCDCONTROLLER_H_ */
