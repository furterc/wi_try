/*
 * LCDPCF8574.cpp
 *
 *  Created on: 20 Oct 2018
 *      Author: christo
 */

#include <LCDPCF8574.h>

LCDPCF8574::LCDPCF8574(I2C *i2c, uint8_t deviceAddress) : PCF8574(i2c, deviceAddress), mDataPort(0)
{

}

LCDPCF8574::~LCDPCF8574()
{

}

void LCDPCF8574::init(uint8_t dispAttr)
{
    mDataPort = 0;
    setOutput(mDataPort);

    // 20ms delay for startup
    HAL_Delay(20);

    /* initial write to lcd is 8bit */
    mDataPort |= (1 << LCD_DATA1_PIN);  // _BV(LCD_FUNCTION)>>4;
    mDataPort |= (1 << LCD_DATA0_PIN);  // _BV(LCD_FUNCTION_8BIT)>>4;
    setOutput(mDataPort);

    toggleEnable();
    HAL_Delay(5);

    toggleEnable();
    HAL_Delay(1);

    toggleEnable();
    HAL_Delay(1);

    /* now configure for 4bit mode */
    mDataPort &= ~(1 << LCD_DATA0_PIN);
    setOutput(mDataPort);
    toggleEnable();
    HAL_Delay(1);

    /* from now the LCD only accepts 4 bit I/O, we can use lcd_command() */

    command(LCD_FUNCTION_4BIT_2LINES);      /* function set: display lines  */

    command(LCD_DISP_OFF);              /* display off                  */
    clrscr();                           /* display clear                */
    command(LCD_MODE_DEFAULT);          /* set entry mode               */
    command(dispAttr);                  /* display/cursor control       */

}

void LCDPCF8574::clrscr(void)
{
    command(1<<LCD_CLR);
}

void LCDPCF8574::home()
{
    command(1<<LCD_HOME);
}

void LCDPCF8574::led(uint8_t onoff)
{
    if(onoff)
        mDataPort &= ~(1 << LCD_LED_PIN);
    else
        mDataPort |= (1 << LCD_LED_PIN);
    setOutput(mDataPort);
}

void LCDPCF8574::gotoxy(uint8_t x, uint8_t y)
{
#if LCD_LINES==1
    lcd_command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
#endif
#if LCD_LINES==2
    if ( y==0 )
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
    else
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE2+x);
#endif
#if LCD_LINES==4
    if ( y==0 )
        command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
    else if ( y==1)
        command((1<<LCD_DDRAM)+LCD_START_LINE2+x);
    else if ( y==2)
        command((1<<LCD_DDRAM)+LCD_START_LINE3+x);
    else /* y==3 */
        command((1<<LCD_DDRAM)+LCD_START_LINE4+x);
#endif

}/* lcd_gotoxy */

void LCDPCF8574::putc(char c)
{
    uint8_t pos;

    pos = waitbusy();   // read busy-flag and address counter
    if (c=='\n')
    {
        newline(pos);
    }
    else
    {
#if LCD_WRAP_LINES==1
#if LCD_LINES==1
        if ( pos == LCD_START_LINE1+LCD_DISP_LENGTH ) {
            write((1<<LCD_DDRAM)+LCD_START_LINE1,0);
        }
#elif LCD_LINES==2
        if ( pos == LCD_START_LINE1+LCD_DISP_LENGTH ) {
            write((1<<LCD_DDRAM)+LCD_START_LINE2,0);
        }else if ( pos == LCD_START_LINE2+LCD_DISP_LENGTH ){
            write((1<<LCD_DDRAM)+LCD_START_LINE1,0);
        }
#elif LCD_LINES==4
        if ( pos == LCD_START_LINE1+LCD_DISP_LENGTH ) {
            write((1<<LCD_DDRAM)+LCD_START_LINE2,0);
        }else if ( pos == LCD_START_LINE2+LCD_DISP_LENGTH ) {
            write((1<<LCD_DDRAM)+LCD_START_LINE3,0);
        }else if ( pos == LCD_START_LINE3+LCD_DISP_LENGTH ) {
            write((1<<LCD_DDRAM)+LCD_START_LINE4,0);
        }else if ( pos == LCD_START_LINE4+LCD_DISP_LENGTH ) {
            write((1<<LCD_DDRAM)+LCD_START_LINE1,0);
        }
#endif
        waitbusy();
#endif
        write(c, 1);
    }
}

void LCDPCF8574::puts(const char *s)
/* print string on lcd (no auto linefeed) */
{
    char c;
    while ( (c = *s++) ) {
        putc(c);
    }

}

void LCDPCF8574::newline(uint8_t pos)
{
    uint8_t addressCounter;

#if LCD_LINES==1
    addressCounter = 0;
#endif
#if LCD_LINES==2
    if ( pos < (LCD_START_LINE2) )
        addressCounter = LCD_START_LINE2;
    else
        addressCounter = LCD_START_LINE1;
#endif
#if LCD_LINES==4
    if ( pos < LCD_START_LINE3 )
        addressCounter = LCD_START_LINE2;
    else if ( (pos >= LCD_START_LINE2) && (pos < LCD_START_LINE4) )
        addressCounter = LCD_START_LINE3;
    else if ( (pos >= LCD_START_LINE3) && (pos < LCD_START_LINE2) )
        addressCounter = LCD_START_LINE4;
    else
        addressCounter = LCD_START_LINE1;
#endif
    command((1<<LCD_DDRAM)+addressCounter);

}/* lcd_newline */

void LCDPCF8574::command(uint8_t cmd)
{
    waitbusy();
    write(cmd,0);
}

void LCDPCF8574::toggleEnable()
{
    setOutputPinHigh(LCD_E_PIN);
    wait_ms(LCD_EN_DELAY);
    setOutputPinLow(LCD_E_PIN);
}

void LCDPCF8574::write(uint8_t data,uint8_t rs)
{
    if (rs) /* write data        (RS=1, RW=0) */
        mDataPort |= (1 << LCD_RS_PIN);
    else /* write instruction (RS=0, RW=0) */
        mDataPort &= ~(1 << LCD_RS_PIN);
    mDataPort &= ~(1 << LCD_RW_PIN);
    setOutput(mDataPort);

    /* output high nibble first */
    mDataPort &= ~(1 << LCD_DATA3_PIN);
    mDataPort &= ~(1 << LCD_DATA2_PIN);
    mDataPort &= ~(1 << LCD_DATA1_PIN);
    mDataPort &= ~(1 << LCD_DATA0_PIN);
    if(data & 0x80) mDataPort |= (1 << LCD_DATA3_PIN);
    if(data & 0x40) mDataPort |= (1 << LCD_DATA2_PIN);
    if(data & 0x20) mDataPort |= (1 << LCD_DATA1_PIN);
    if(data & 0x10) mDataPort |= (1 << LCD_DATA0_PIN);
    setOutput(mDataPort);
    toggleEnable();

    /* output low nibble */
    mDataPort &= ~(1 << LCD_DATA3_PIN);
    mDataPort &= ~(1 << LCD_DATA2_PIN);
    mDataPort &= ~(1 << LCD_DATA1_PIN);
    mDataPort &= ~(1 << LCD_DATA0_PIN);
    if(data & 0x08) mDataPort |= (1 << LCD_DATA3_PIN);
    if(data & 0x04) mDataPort |= (1 << LCD_DATA2_PIN);
    if(data & 0x02) mDataPort |= (1 << LCD_DATA1_PIN);
    if(data & 0x01) mDataPort |= (1 << LCD_DATA0_PIN);
    setOutput(mDataPort);
    toggleEnable();

    /* all data pins high (inactive) */
    mDataPort |= (1 << LCD_DATA0_PIN);
    mDataPort |= (1 << LCD_DATA1_PIN);
    mDataPort |= (1 << LCD_DATA2_PIN);
    mDataPort |= (1 << LCD_DATA3_PIN);
    setOutput(mDataPort);
}

uint8_t LCDPCF8574::read(uint8_t rs)
{
    uint8_t data;

    if (rs) /* write data        (RS=1, RW=0) */
        mDataPort |= (1 << LCD_RS_PIN);
    else /* write instruction (RS=0, RW=0) */
        mDataPort &= ~(1 << LCD_RS_PIN);
    mDataPort |= (1 << LCD_RW_PIN);
    setOutput(mDataPort);

    setOutputPinHigh(LCD_E_PIN);
    HAL_Delay(LCD_EN_DELAY);
    data = getOutputPin(LCD_DATA0_PIN) << 4;     /* read high nibble first */
    setOutputPinLow(LCD_E_PIN);

    HAL_Delay(LCD_EN_DELAY);                     /* Enable 500ns low       */

    setOutputPinHigh(LCD_E_PIN);
    HAL_Delay(LCD_EN_DELAY);
    data |= getOutputPin(LCD_DATA0_PIN) &0x0F;    /* read low nibble        */
    setOutputPinLow(LCD_E_PIN);

    return data;
}

uint8_t LCDPCF8574::waitbusy()
{
    register uint8_t c;

    /* wait until busy flag is cleared */
    while ( (c=read(0)) & (1<<LCD_BUSY)) {}

    /* the address counter is updated 4us after the busy flag is cleared */
    wait_us(100);

    /* now read the address counter */
    return (read(0));  // return address counter

}
