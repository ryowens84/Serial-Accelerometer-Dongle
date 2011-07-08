//ADC Library for the AtMega368
//Written by Ryan Owens

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/io.h>
#include "adc.h"

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

//Function: adcRead(char) - Reads the ADC count value of a specified ADC channel
//Inputs: char channel - The ADC channel to be read
//Outputs: None
//Return: The adc count of the channel
//Usage: adcRead(4);
unsigned int adcRead(char channel)
{
	unsigned int adc_value=0;
	
	ADMUX = (ADMUX & 0xF0);	//Mask OFF the previous ADC channel
	ADMUX |= (channel & 0x0F);		//Set the new ADC channel

	sbi(ADCSRA, ADSC);	//Start a conversion	
	while(bit_is_set(ADCSRA, ADSC));//Wait for the conversion to finish
	
	adc_value=ADCL;				//Get the lowest 8 bits of the 10 bit conversion
	adc_value |= (ADCH << 8);	//Get the upper 2 bits of the 10 bit conversion
	
	return adc_value;			//Send the value back to the calling function
}

//Function: adcVoltage(unsigned int) - Converts an ADC count to millivolts assuming a 3.3V reference voltage
//Inputs: unsigned int adc_value - The ADC count to be converted
//Outputs: None
//Return: The voltage in millivolts of the ADC count
//Usage: adcVoltage(3500);
unsigned long adcVoltage(unsigned int adc_value)
{
	unsigned long voltage=0;
	
	//Convert the ADC count to a voltage in mV assuming the reference voltage is 3.3V
	voltage = (adc_value * 3300);
	voltage = voltage/1024;
	
	return voltage;
	
}

//Function: adcInit(char, char, char) - Initializes the Analog to Digital Converter with the input settings
//Inputs: reference - The reference voltage for the ADC (0, 1 or 3 accepted)
//		  align - The desired adjustment for the ADC result (0 or 1 accepted)
//		  clock - The division factor for to produce the ADC clock (0 - 7 accepted)
//Outputs: None
//Return: None
//Usage: adcInit(3, 0, 7);
void adcInit(char reference, char align)
{
	ADMUX = (reference << REFS0);	//Shift the reference voltage into the ADMUX register
	ADMUX |= (align << ADLAR);		//Shift the left adjust into the ADMUX register
	
	//ADCSRA = clock;		//Set the ADC clock prescaler
	
	// set a2d prescale factor to 64
	// 8 MHz / 64 = 125 KHz, inside the desired 50-200 KHz range.
	// TODO: this will not work properly for other clock speeds, and
	// this code should use F_CPU to determine the prescale factor.
	sbi(ADCSRA, ADPS2);
	sbi(ADCSRA, ADPS1);
	//sbi(ADCSRA, ADPS0);
	

	// enable a2d conversions
	sbi(ADCSRA, ADEN);	
}

//Description: Puts the ADC into free running mode.
//Notes: This will trigger successive adc reads. The ADC Interrupt Flag can be used
// to trigger an interrupt in order to manipulate the ADC data.
// The read time for a single read depends on F_CPU and the ADC Prescalar value, but 
// is approximately 14 clock cycles.
void adcFreeRunning(char active)
{
	if(active != 0)	
	{
		sbi(ADCSRA, ADATE); //Enable automatic triggering
		sbi(ADCSRA, ADIE);	//Enable ADC Interrupts (Interrupt will be triggered after every read)
		sbi(ADCSRA, ADSC);	//Start the free running readings (Interrupts will not start until sei() is called);
	}
	else{
		cbi(ADCSRA, ADATE);
		cbi(ADCSRA, ADIE);
		cbi(ADCSRA, ADSC);
	}
}