/*********************************************
* Timer 2 Library for the ATmega328
*
* Contains general functions for initializing
* and using the 8 bit time 2
*
* (i.e. setting up interrupts, keeping track
*  of total program duration, setting delays
*  and alarms.
**********************************************/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/io.h>
#include "timer2.h"

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

volatile unsigned long elapsedMillis;

//Description: Initializes timer 2 for 8 MHz timer and enables overflow interrupts
// TODO: Make this function use the F_CPU variable to configure the timer.
void timer2Init(void)
{
	//Set the initial timer value so the overflow interrupts will trigger at 1ms.
	TCNT2 = 5;

	// Init timer 2
	//Set Prescaler to 1. (Timer Frequency set to F_CPU)
	TCCR2B = ((1<<CS20)|(1<<CS21)); 	//Divde clock by 32
	sbi(TIMSK2, TOIE2);	//Enable Timer Overflow interrupts;
}

//General short delays
void delayMs(uint16_t x)
{
    unsigned long delay = millis() + x;
	while(millis() < delay);
}

//Description: Returns the elapsed milliseconds since the device was powered up
// Maximum value ~ 1193 hours, or 49 days
unsigned long millis(void)
{
	return elapsedMillis;
}