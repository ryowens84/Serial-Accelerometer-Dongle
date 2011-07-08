/******************************************************
*
* EEPROM Library Header File for ATmega328
*
* General functions for reading and writing to EEPROM
* (Sample code taken from the Atmel Datasheet)
* Modified by Ryan Owens
* 6/17/2011
*
*******************************************************/
unsigned char eepromReadChar(unsigned int uiAddress);
void eepromWriteChar(unsigned int uiAddress, unsigned char ucData);
unsigned int eepromReadInt(unsigned int uiAddress);
void eepromWriteInt(unsigned int uiAddress, unsigned int uiData);
unsigned long eepromReadLong(unsigned int uiAddress);
void eepromWriteLong(unsigned int uiAddress, unsigned long ulData);