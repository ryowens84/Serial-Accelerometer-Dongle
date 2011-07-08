/******************************************************
*
* EEPROM Library for ATmega328
*
* General functions for reading and writing to EEPROM
* (Sample code taken from the Atmel Datasheet)
* Modified by Ryan Owens
* 6/17/2011
*
*******************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/io.h>
#include "eeprom.h"

unsigned char eepromReadChar(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from Data Register */
	return EEDR;
}

void eepromWriteChar(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address and Data Registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR|= (1<<EEPE);
}

//Description: Reads an unsigned integer from the specified address of EEPROM
//	Assumes that the value is stored with the MSB in the given address, and the LSB in the following address (Big Endian)
unsigned int eepromReadInt(unsigned int uiAddress)
{
	unsigned int tempValue=0;
	
	tempValue = eepromReadChar(uiAddress); //Read the first byte of data
	tempValue = tempValue << 8;	//Move the data to the upper byte of the integer
	
	tempValue |= eepromReadChar(uiAddress+1);	//Get the second byte of data
	
	return tempValue;
}

//Description: Writes an unsigned integer to the specified address of EEPROM
// The value is written in Big Endian form, so the MSB will be stored in the address given, and the LSB will be stored in the following address
void eepromWriteInt(unsigned int uiAddress, unsigned int uiData)
{
	unsigned char partialValue=0;
	partialValue = uiData >> 8;
	eepromWriteChar(uiAddress, partialValue);	//Store the upper byte of the data.
	partialValue = (unsigned char)uiData;
	eepromWriteChar(uiAddress + 1, partialValue);	//Store the lower byte of data in the following address
}

//Description: Reads an unsigned long value from the specified address of EEPROM
// Assumes that the value is stored in Big Endian format.
unsigned long eepromReadLong(unsigned int uiAddress){
	unsigned long tempValue=0;
	tempValue = eepromReadInt(uiAddress); //Read the first 2 bytes of data	
	tempValue = tempValue << 16;	//Shift the first two bytes of data into the upper section of the temp Value.
	tempValue |= eepromReadInt(uiAddress + 2);	//Get the second 2 bytes of data and put them in the lower half of the temp value.
	
	return tempValue;
}

//Description: Writes an unsigned long value to the specified addres in EEPROM
// The value will be stored in Big Endian format.
void eepromWriteLong(unsigned int uiAddress, unsigned long ulData)
{
	unsigned int partialValue=0;
	partialValue = ulData >> 16;	//Get the 2 high bytes from the data to be stored.
	eepromWriteInt(uiAddress, partialValue);
	
	partialValue = (unsigned int)ulData;	//Get the 2 lower bytes from the data to be stored.
	eepromWriteInt(uiAddress + 2, partialValue);
}
