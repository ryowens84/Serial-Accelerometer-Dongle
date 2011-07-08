//ADC Library for the AtMega368
//Written by Ryan Owens

//Constant definitions for the ADC Init. function
#define AREF	0
#define AVCC	1
#define INTERNAL	3

#define LEFT	1
#define RIGHT	0

unsigned int adcRead(char channel);
unsigned long adcVoltage(unsigned int adc_value);
void adcInit(char reference, char align);
void adcFreeRunning(char active);

#define toVoltage(count, voltage)	voltage = count * 3300 / 1023