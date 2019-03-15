/*
 * LCDController.cpp
 *
 *  Created on: 20 Nov 2018
 *      Author: christo
 */

#include <mbed.h>
#include <LCDController.h>
#include <utils.h>

#define TRACE(_x, ...) INFO_TRACE("LCD", _x, ##__VA_ARGS__)


LCDController::LCDController(LCDPCF8574 *lcd) : mLcd(lcd), mNextTick(0)
{
    linked_list_Init(&mLinkedList, LCD_LOG_ENTRIES);

    lcd->init(LCD_DISP_ON);
    printf(GREEN("Initialized\n"));

    lcd->led(1);
    lcd->gotoxy(10, 0);
    lcd->puts("hi");

    for(int i = 0; i < 3; i++)
        lcdLines[i] = 0;

    TRACE(GREEN("initialized"));
}

void LCDController::writeLine(char *data, uint8_t line)
{
    if(strlen(data) > 20)
        return;

    if(line > 3)
        return;

    char buf[21];
    memset(buf,' ', 20);
    buf[20] = 0;
    memcpy(buf, data, strlen(data));

    mLcd->gotoxy(0, line);
    mLcd->puts(buf);
}

int LCDController::logLine(char *data)
{
    if(strlen(data) > 20)
        return -1;

    uint8_t len = strlen(data);
    char *entry = (char*)malloc(len+1);
    memcpy(entry, data, len);
//    printf("\n\nlog add @ %p = %s\n", entry, data);
    entry[len] = 0;

    for(int i = 0; i < 3; i++)
    {
        if(lcdLines[i] == 0)
        {
            if(i == 0)
                mLcd->led(0);

            lcdLines[i] = entry;

            if(mNextTick == 0)
            {
                mNextTick = HAL_GetTick() + LCD_LOG_TIMEOUT;
            }

            logUpdateDisplay();
            return 0;
        }
    }

    if(linked_list_Append(&mLinkedList, entry, len) != HAL_OK)
    {
        TRACE("LOG add failed\n");
        free(entry);
        return -1;
    }
    return 0;
}

void LCDController::logUpdateDisplay()
{
    for(uint8_t idx = 0; idx < 3; idx++)
    {
        if(lcdLines[idx] != 0)
            writeLine(lcdLines[idx], idx);
        else
            clearLine(idx);
    }
}

void LCDController::logUpdate()
{
//    for(int i = 0; i < 3; i++)
//    {
//        printf("lcdLines[%d] %p\n", i, lcdLines[i]);
//        if(lcdLines[i] != 0)
//            printf("lcdLines[%d] = %s\n", i, lcdLines[i]);
//    }

    if(lcdLines[0] == 0)
    {
        mLcd->led(1);
        mNextTick = 0;
        return;
    }

//    printf("free lcd line @ %p \n", lcdLines[0]);
    free(lcdLines[0]);
    lcdLines[0] = 0;

    if(lcdLines[1] == 0)
    {
        logUpdateDisplay();
        return;
    }

    lcdLines[0] = lcdLines[1];
    lcdLines[1] = 0;

    if(lcdLines[2] == 0)
    {
        logUpdateDisplay();
        return;
    }

    lcdLines[1] = lcdLines[2];
    lcdLines[2] = 0;

    char *data = 0;
    if(linked_list_Pop(&mLinkedList, (void **)&data) == -1)
    {
        printf(RED("log pop fail\n"));
        logUpdateDisplay();
        return;
    }

//    printf("log pop @ %p = %s\n", data, data);
    lcdLines[2] = data;
    logUpdateDisplay();
}

void LCDController::logRun()
{
    if(!mNextTick)
        return;

    if(mNextTick < HAL_GetTick())
    {
        mNextTick = HAL_GetTick() + LCD_LOG_TIMEOUT;

        logUpdate();
    }
}

void LCDController::clearLine(uint8_t line)
{
    if(line > 3)
        return;

    char data[21];
    memset(data, ' ', 20);
    data[20] = 0;
    mLcd->gotoxy(0, line);
    mLcd->puts(data);
}

LCDController::~LCDController()
{

}

