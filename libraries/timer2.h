/*********************************************
* Timer 2 Library Header Filefor the ATmega328
*
* Contains general functions for initializing
* and using the 8 bit time 2
*
* (i.e. setting up interrupts, keeping track
*  of total program duration, setting delays
*  and alarms.
**********************************************/
void timer2Init(void);
void delayMs(uint16_t x);
void delayUs(uint16_t x);
unsigned long millis(void);

extern volatile unsigned long elapsedMillis;