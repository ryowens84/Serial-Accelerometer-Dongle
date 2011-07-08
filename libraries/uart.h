/*********************************************************
* UART Library Header File for ATmega328
* General functions for configuring the UART and
* reading/writing characters from the UART
*
* Written by Ryan Owens
* 6/15/11
*********************************************************/
int uartInit(unsigned long baudRate);
int uartPutchar(char c, FILE *stream);
uint8_t uartGetChar(void);
static FILE mystdout = FDEV_SETUP_STREAM(uartPutchar, NULL, _FDEV_SETUP_WRITE);