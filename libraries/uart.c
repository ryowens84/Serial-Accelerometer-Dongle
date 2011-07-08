/*********************************************************
* UART Library for ATmega328
* General functions for configuring the UART and
* reading/writing characters from the UART
*
* Written by Ryan Owens
* 6/15/11
*********************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <avr/io.h>
#include <math.h>
#include "uart.h"

//Expects F_CPU to be defined as the system clock frequency (in Hz) in the Makefile
int uartInit(unsigned long baudRate){
	//This equation needs to be fixed
	unsigned int myUbrr = (unsigned int)round((double)((F_CPU/16)/(double)baudRate*2-1));
	
	UBRR0H = (myUbrr >> 8) & 0x7F;	//Make sure highest bit(URSEL) is 0 indicating we are writing to UBRRH
	UBRR0L = myUbrr;
	UCSR0A = (1<<U2X0);					//Double the UART Speed
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);		//Enable Rx and Tx in UART
	UCSR0C = (1<<UCSZ00)|(1<<UCSZ01);		//8-Bit Characters
	stdout = &mystdout; //Required for printf init

	return myUbrr;
}

int uartPutchar(char c, FILE *stream)
{ 
  loop_until_bit_is_set(UCSR0A, UDRE0);
  UDR0 = c;
  return 0;
}

uint8_t uartGetChar(void)
{
    while( !(UCSR0A & (1<<RXC0)) );
	return(UDR0);
}