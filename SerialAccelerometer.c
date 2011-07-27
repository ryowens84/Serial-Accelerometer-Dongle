#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "SerialAccelerometer.h"
#include "adc.h"
#include "uart.h"
#include "timer2.h"
#include "eeprom.h"

//================================================================
//Define Global Variables
//================================================================
char tempCharacter=0;
char firstRun=0;
volatile unsigned int currentAxis = Z_AXIS, currentReading=0;
//ADC Reading array will hold the last NUM_READINGS adc values for each axis.
volatile unsigned int adcReading[3][NUM_READINGS];
volatile bool blinkOn = false;

//This is a list of the possible baud rates, chosen by the baudRate setting
const unsigned long baudRateSettings[7] = {4800, 9600, 14400, 19200, 38400, 57600, 115200};
//This is a list of the output frequency limits, which are based on the output mode and baud rate.
//Limits were found by experimental testing, and are limited to 250 Hz to ensure that all 3 axis are read
// before displaying the values.
//There is a seperate list of limits for each output mode, and each list contains a limit for every possible baud rate.
//TODO: Make this be a calculation instead of a list.
const unsigned long outputFrequencyLimits[3][7] = {
{25, 45, 66, 83, 125, 142, 166},
{27, 58, 76, 111, 200, 250, 250}, 
{47, 90, 125, 166, 250, 250, 250}
};

/**************************************************************
* Define Interrupt Subroutines
**************************************************************/
ISR(ADC_vect)
{
	cli();
	//Get the value from the ADC
	adcReading[currentAxis][currentReading%NUM_READINGS]=ADCL;				//Get the lowest 8 bits of the 10 bit conversion
	adcReading[currentAxis][currentReading%NUM_READINGS] |= (ADCH << 8);	//Get the upper 2 bits of the 10 bit conversion

	//Update the axis (Read each axis before updating the currentReading parameter.)
	if(currentAxis == Z_AXIS)
	{
		currentAxis = X_AXIS;
		currentReading++;
	}
	else currentAxis--;
	
	//Update the ADC Channel to get the value of the next axis
    ADMUX = (ADMUX & 0xF0);	//Mask OFF the previous ADC channel
	ADMUX |= (currentAxis & 0x0F);		//Set the new ADC channel	
	
	sei();
}

//Description: Timer 2 overflow interrupt keeps track of elapsed milliseconds
//Note: timer 2 can't be used by anything else now!
// Also, this ISR assumes an 8 MHz F_CPU
//TODO: Move this to another interrupt vector so that timer 2 can be shared.
ISR(TIMER2_OVF_vect)
{
	cli();
	
	elapsedMillis++;	//Increment the millisecond timer
	TCNT2 = 5;
	
	if(blinkOn && (elapsedMillis % 100==0))ledToggle();
	
	sei();
}

int main (void)
{
	/**************************************************************
	* Define Local Variables
	**************************************************************/
	//Create structures that will hold the ADC counts of the MMA7361 axis measurements
	struct sensorReadings sensorADCCount;
	//Create a structure for the sensor voltages (stored in millivolts)
	struct sensorReadings sensorVoltage;
	//Create a structure that will hold the calibration values, aka 0g offset values (stored in millivolts)
	struct sensorReadings sensorCalibration;
	//Create a structure that will hold the millivolt 'swing' for each axis (i.e. number of millivolts that represent 1g to -1g)
	//(used for calculating the G Value)
	struct sensorReadings sensorSwing;
	//Create a structure that will hold the g value data for the MMA7361 data (X, Y and Z axis)
	struct sensorValues sensorG;
	//Create a structure to hold the configuration settings.
	struct settings mySettings;

	//Run program will keep the device in a 'measurement mode.'
	bool runProgram = false;
	//This variable will hold the current menu selection entered by the user.
	char menuSelection = 0;
	//This variable will hold the maximum output period (converted from frequency) for the current output mode/baud rate combination
	unsigned long outputPeriod = 0, currentTime=0;
	
	//Testing Variable
	int testValue=1;

	/**************************************************************
	* Run Initialization Sequences
	**************************************************************/
	//Initialize AVR I/O, UART and Interrupts
    IOInit();
	ledOff();
	//Initialize Timer 2 to utilize the delay routines
	timer2Init();
	//Initialize the ADC module
	adcInit(AVCC, RIGHT);
	
	//Find out if this is the first time the board has run!
	firstRun = eepromReadChar(0);
	//If the BOOT_RESET pin is low or this is the first run, we need to load the factory settings and run the test procedure
	if((!(PINC & (1<<BOOT_RESET))) || (firstRun != 0)){
		//Set the settings to the 'factory defaults'
		mySettings.accelerometerRange = RANGE_15;
		mySettings.outputMode = OUTPUT_GRAVITY;
		mySettings.outputFrequency = 50;
		mySettings.baudRate = BAUD_38400;
		saveSettings(&mySettings);
		
		//Set the calibration values to the MMA7361 recomended values
		sensorCalibration.x=1650;
		sensorCalibration.y=1650;
		sensorCalibration.z=1650;
		saveCalibration(&sensorCalibration);
		
		//Set the swing values for each axis to the estimated values in 1.5g mode from the MMA7361 datasheet
		sensorSwing.x = RANGE_15;
		sensorSwing.y = RANGE_15;
		sensorSwing.z = RANGE_15;
		saveSwing(&sensorSwing);
		
		//Write a 0 to the first run memory location.
		eepromWriteChar(0, 0);
		
		//Now run the test procedure
		blinkOn = true;
		uartInit(38400);
		sei();
		
		printf("Testing Accelerometer...\n\r");
		delayMs(1000);
		
		//Read each axis of the accelerometer
		sensorADCCount.x = adcRead(X_AXIS);
		sensorADCCount.y = adcRead(Y_AXIS);
		sensorADCCount.z = adcRead(Z_AXIS);	

		//Convert the values to Voltages
		toVoltage(sensorADCCount.x, sensorVoltage.x);
		toVoltage(sensorADCCount.y, sensorVoltage.y);
		toVoltage(sensorADCCount.z, sensorVoltage.z);				
		//Finally convert the voltages to Gs
		toGValue(&sensorG, &sensorVoltage, &sensorCalibration, &sensorSwing);	
		
		//Now check to see if the sensor values are within range
		//Check X Axis
		if((sensorG.x >= 0.2) || (sensorG.x <= -0.2)){
			printf("X Axis Fails!\n\r");
			testValue=0;
		}
		if((sensorG.y >= 0.2) || (sensorG.y <= -0.2)){
			printf("Y Axis Fails!\n\r");
			testValue=0;
		}		
		if((sensorG.z >= 1.2) || (sensorG.z <= 0.8)){
			printf("Z Axis Fails!\n\r");
			testValue=0;
		}	
		if(testValue == 1){
			printf("Pass\n\r");
			blinkOn = false;
			ledOn();
		}
		while(1);
		
	}
	//Otherwise, load settings from EEPROM
	else{
		loadSettings(&mySettings);
		loadCalibration(&sensorCalibration);
		loadSwing(&sensorSwing);
	}
	
	//Use the settings to configure the device 	
	uartInit(baudRateSettings[mySettings.baudRate]);
	setAccelerometerRange(mySettings.accelerometerRange);
	
	/***************************************************************
	* Run the main code
	***************************************************************/
	//Interrupts must be enabled for the millis() and delay() functions to work.
	//The free running ADC also will not work without interrupts enabled.
	sei();
	while(1){
		/************************************************************************
		* Configuration Mode
		* Device will stay in configuration mode until the exit option ('x')
		* is chosen.
		************************************************************************/
		ledOn();
		//Take the ADC Module out of free running mode
		adcFreeRunning(0);
		//Make sure the program is not in run mode (unless set in the menu)
		runProgram = false;
		//Keep displaying the configuration menu until a valid option is selected
		menuSelection = configMenu(&mySettings, &sensorCalibration);
		while(((menuSelection < '1') || (menuSelection > '5')) && (toupper(menuSelection) != 'X')) {
			printf("Invalid Selection!\n\r");
			menuSelection = configMenu(&mySettings, &sensorCalibration);
		}
		printf("%c\n\n\r", menuSelection);
		switch(toupper(menuSelection)){
			case MENU_CALIBRATE: 
				//Lead the user through calibrating the sensor
				//May also need to record the 1g 'swing' (or the actual millivolts per G) along with the 0g value
				selectCalibrationValues(&sensorCalibration, &sensorSwing);
				//Save the new calibration values
				cli();
				saveCalibration(&sensorCalibration);
				saveSwing(&sensorSwing);
				sei();
				break;
			case MENU_MODE:
				//Prompt the user to select the desired output mode (G value, Raw Data, or Raw Data in Binary Format)
				//Output mode will be used in Measurement mode to determine which format to output the data in
				selectOutputMode(&mySettings);
				break;
			case MENU_FREQUENCY:
				//Prompt the user to select the output frequency.
				//User is limited to upper bounds based on output format and the current baud rate.
				//This value will determine the ADC read frequencies as well as the output frequency.
				selectOutputFrequency(&mySettings);
				break;
			case MENU_RANGE:
				//Prompt the user to select the G range of the MMA7361 (+/-1.5G or +/-6.0g)
				selectAccelerometerRange(&mySettings, &sensorSwing);
				setAccelerometerRange(mySettings.accelerometerRange);
				break;
			case MENU_BAUD:
				//Prompt the user for the new baud rate setting
				selectBaudRate(&mySettings);
				//Reinitialize the UART for the new baud rate
				uartInit(baudRateSettings[mySettings.baudRate]);
				break;
			case MENU_EXIT:
				//If the user exits the configuration menu, the device will enter measurement mode.
				runProgram = true;
				break;
		}
		//Check to see if the new settings caused the current output frequency to exceed the maximum value
		//If the maximum frequency has been exceeded, limit it and notify the user!
		if(mySettings.outputFrequency > outputFrequencyLimits[mySettings.outputMode][mySettings.baudRate])
		{
			printf("The new settings have caused the output frequency to change.\n\n\r");
			mySettings.outputFrequency = outputFrequencyLimits[mySettings.outputMode][mySettings.baudRate];
		}
		//Always save the settings after exiting the configuration menu, just in case something changed
		cli();
		saveSettings(&mySettings);
		sei();	
	
		/**************************************************************
		* Measurement Mode
		* The device will stay in this mode until a key is pressed
		**************************************************************/
		if(runProgram){
			//Set up the ADC to start reading from the X axis
			currentAxis = X_AXIS;
			currentReading = 0;
			adcRead(currentAxis);	//Set the ADMUX Registers to read from the X Axis
			//Put the ADC Module into free running mode
			adcFreeRunning(1);
			
			//Figure out what the output period should be, given the output frequency
			outputPeriod = 1000/mySettings.outputFrequency;	//Find the period in ms.
			//Used for Debug Purposes
			//printf("Output Peridod: %lu\n\r", outputPeriod);
			//delayMs(1000);
			
			//Wait to get at least NUM_READINGS so we can average the readings properly
			while(currentReading < NUM_READINGS);
		}
		
		while(runProgram){	
			//Clear out the last value
			sensorADCCount.x=0;
			sensorADCCount.y=0;
			sensorADCCount.z=0;
		
			//Get the continuous average of the last NUM_READINGS adc readings for each axis
			//Pause interrupts while we do this.
			adcFreeRunning(0);
			for(int readingNumber=0; readingNumber < NUM_READINGS; readingNumber++)
			{
				sensorADCCount.x += adcReading[X_AXIS][readingNumber];
				sensorADCCount.y += adcReading[Y_AXIS][readingNumber];
				sensorADCCount.z += adcReading[Z_AXIS][readingNumber];
			}
			adcFreeRunning(1);
			
			sensorADCCount.x /= NUM_READINGS;
			sensorADCCount.y /= NUM_READINGS;
			sensorADCCount.z /= NUM_READINGS;
			
			currentTime = millis();
			if((currentTime % outputPeriod) <= 1){	
				ledToggle();
				if(mySettings.outputMode == OUTPUT_GRAVITY){
					//Convert the values to Voltages
					toVoltage(sensorADCCount.x, sensorVoltage.x);
					toVoltage(sensorADCCount.y, sensorVoltage.y);
					toVoltage(sensorADCCount.z, sensorVoltage.z);				
					//Finally convert the voltages to Gs
					toGValue(&sensorG, &sensorVoltage, &sensorCalibration, &sensorSwing);
					printf("% 05.2f\t% 05.2f\t% 05.2f\n\r", sensorG.x, sensorG.y, sensorG.z);
				}
				else if(mySettings.outputMode == OUTPUT_RAW){
					printf("%04ld\t%04ld\t%04ld\n\r", sensorADCCount.x, sensorADCCount.y, sensorADCCount.z);
				}
				else if(mySettings.outputMode == OUTPUT_BINARY){
					printf("#%c%c%c%c%c%c$",
						(char)(sensorADCCount.x>>8), (char)sensorADCCount.x,
						(char)(sensorADCCount.y>>8), (char)sensorADCCount.y,
						(char)(sensorADCCount.z>>8), (char)sensorADCCount.z);
				}
			}
			if(UCSR0A & (1<<RXC0)){
				tempCharacter = UDR0;
				runProgram = false;
			}			
		}
	}
	
    return (0);
}

//==================================================
//Core functions
//==================================================
void IOInit(void)
{
	DDRD |= (1<<G_SELECT);	//Set the GS pin as an output
	
	DDRC &= ~((1<<X_AXIS)|(1<<Y_AXIS)|(1<<Z_AXIS)|(1<<BOOT_RESET));	//Set AX, AY and AZ as inputs
	PORTC |= (1<<BOOT_RESET);	//Enable the pull-up resistor on the boot reset pin
	
	DDRB |= (1<<LED_PIN);	//Set the LED as an output pin.
}


void selectAccelerometerRange(struct settings* newSettings, struct sensorReadings* swingValues)
{
	char tempValue=0;
	
	printf("Select the desired accelerometer range.\n\r");
	printf("[1] +/- 1.5g\n\r");
	printf("[2] +/- 6.0g\n\r");
	tempValue = uartGetChar();
	
	switch(tempValue){
		case '1':
			newSettings->accelerometerRange = RANGE_15;
			//Reinitialize the swing values to default for this mode
			//TODO: Make seperate swing settings for high/low ranges
			swingValues->x=RANGE_15;
			swingValues->y=RANGE_15;
			swingValues->z=RANGE_15;
			break;
		case '2':
			newSettings->accelerometerRange = RANGE_60;
			//Reinitialize the swing values to default for this mode
			//TODO: Make seperate swing settings for high/low ranges
			swingValues->x=RANGE_60;
			swingValues->y=RANGE_60;
			swingValues->z=RANGE_60;			
			break;
		default:
			printf("Invalid Selection");
			break;
	}	
	saveSwing(swingValues);
	printf("\n\n\r");
}

//Description: Displays a configuration menu to the user.
//Parameters: None
//Returns: The configuration menu item selected by the user
//usage: selection = configMenu();
char configMenu(struct settings* menuSettings, struct sensorReadings* menuCalibrationValues)
{
	//Display the Config Menu welcome dialoge
	printf("--- Serial Accelerometer Dongle MMA7361 ---\n\r");
	printf("          Firmware Version 6.0\n\n\r");
	printf("Select a menu item to continue:\n\r");
	//Display the config menu options
	printf("[1] Calibrate (Current Calibration Values: %ld, %ld, %ld)\n\r", menuCalibrationValues->x, menuCalibrationValues->y, menuCalibrationValues->z);
	printf("[2] Output Mode (");
	//Display the current output mode of the accelerometer data
	switch(menuSettings->outputMode){
		case OUTPUT_GRAVITY: printf("Gravity Values");
			break;
		case OUTPUT_RAW: printf("Raw ADC Values");
			break;
		case OUTPUT_BINARY: printf("Raw ADC Values in Binary Format");
			break;
		default:
			break;
	}
	printf(")\n\r");
	printf("[3] Output Frequency (%d Hz)\n\r", menuSettings->outputFrequency);
	printf("[4] Sensor Range (+/- ");
	switch(menuSettings->accelerometerRange){
		case RANGE_60: printf("6.0g");
			break;
		case RANGE_15: printf("1.5g");
			break;
	}
	printf(")\n\r");
	printf("[5] Baud Rate (%lu)\n\r", baudRateSettings[menuSettings->baudRate]);
	printf("[x] Exit\n\r");
	printf("Selection: ");
	
	return uartGetChar();
}

void toGValue(struct sensorValues* gValue, struct sensorReadings* voltage, struct sensorReadings* calibration, struct sensorReadings* swing){
	gValue->x = ((double)(voltage->x) - (double)(calibration->x))/(double)swing->x;
	gValue->y = ((double)(voltage->y) - (double)(calibration->y))/(double)swing->y;
	gValue->z = ((double)(voltage->z) - (double)(calibration->z))/(double)swing->z;
}

//TODO: Make this function smaller
void selectCalibrationValues(struct sensorReadings* newCalibrationValues, struct sensorReadings* swingValues){
	unsigned long int tempMax=0, tempMin=0;
		
	printf("Calibration Menu (Press X at any time to Exit)\n\r");
	printf("For each axis you will be prompted to find the maximum and minimum values.\n\r");
	printf("Simply rotate the serial accelerometer until you find the appropriate value and\n\r");
	printf("press a key (any key except x) to register the value\n\r");
	
	//Calibrate the X Axis
	printf("Calibrate X Axis\n\r");
	printf("Find Maximum X Value:\n\r");
	while( !(UCSR0A & (1<<RXC0)) ){
		tempMax = adcRead(X_AXIS);
		printf("X:\t%lu\r", tempMax);
		delayMs(50);
	}
	if(toupper(UDR0)=='X')return;
	printf("Find Minimum X Value\n\r");
	while( !(UCSR0A & (1<<RXC0)) ){
		tempMin = adcRead(X_AXIS);
		printf("X:\t%lu\r", tempMin);
		delayMs(50);
	}
	if(toupper(UDR0)=='X')return;
	toVoltage(((tempMax - tempMin)/2), swingValues->x);
	toVoltage((((tempMax - tempMin)/2) + tempMin), newCalibrationValues->x);
	
	//Calibrate Y Axis
	printf("Calibrate Y Axis\n\r");
	printf("Find Maximum Y Value:\n\r");
	while( !(UCSR0A & (1<<RXC0)) ){
		tempMax = adcRead(Y_AXIS);
		printf("Y:\t%lu\r", tempMax);
		delayMs(50);
	}
	if(toupper(UDR0)=='X')return;
	printf("Find Minimum Y Value\n\r");
	while( !(UCSR0A & (1<<RXC0)) ){
		tempMin = adcRead(Y_AXIS);
		printf("Y:\t%lu\r", tempMin);
		delayMs(50);
	}
	if(toupper(UDR0)=='X')return;
	toVoltage(((tempMax - tempMin)/2), swingValues->y);
	toVoltage((((tempMax - tempMin)/2) + tempMin), newCalibrationValues->y);
	
	//Calibrate Z Axis
	printf("Calibrate Z Axis\n\r");
	printf("Find Maximum Z Value:\n\r");
	while( !(UCSR0A & (1<<RXC0)) ){
		tempMax = adcRead(Z_AXIS);
		printf("Z:\t%lu\r", tempMax);
		delayMs(50);
	}
	if(toupper(UDR0)=='X')return;
	printf("Find Minimum Z Value\n\r");
	while( !(UCSR0A & (1<<RXC0)) ){
		tempMin = adcRead(Z_AXIS);
		printf("Z:\t%lu\r", tempMin);
		delayMs(50);
	}
	if(toupper(UDR0)=='X')return;
	toVoltage(((tempMax - tempMin)/2), swingValues->z);
	toVoltage((((tempMax - tempMin)/2) + tempMin), newCalibrationValues->z);
	
	printf("\n\n\r");
}

void selectOutputMode(struct settings* newSettings){
	char tempModeSelection=0;
	printf("Select the desired output mode\n\r");
	printf("[1] Gravity Values\n\r");
	printf("[2] Raw Values\n\r");
	printf("[3] Raw Values in Binary Format\n\r");
	tempModeSelection = uartGetChar();
	switch(tempModeSelection){
		case '1':
			newSettings->outputMode = OUTPUT_GRAVITY;
			break;
		case '2':
			newSettings->outputMode = OUTPUT_RAW;
			break;
		case '3':
			newSettings->outputMode = OUTPUT_BINARY;
			break;
		default:
			printf("Invalid Selection.\n\r");
	}
	printf("\n\n\r");
}

void selectOutputFrequency(struct settings* newSettings){
	char tempValue=0;
	
	printf("Set the desired output frequency. Press [i] to increase and [d] to decrease.\n\rPress [x] to exit\n\r");
	printf("Frequency range is limited automatically by the output mode and baud rate\n\r");
	printf("Output Frequency: %3d\r", newSettings->outputFrequency);
	tempValue = uartGetChar();
	while(tolower(tempValue) != 'x'){
		if((tolower(tempValue)=='i') && (newSettings->outputFrequency < outputFrequencyLimits[newSettings->outputMode][newSettings->baudRate]))
			newSettings->outputFrequency += 1;
		if((tolower(tempValue)=='d') && (newSettings->outputFrequency >= 1))newSettings->outputFrequency -= 1;
		printf("Output Frequency: %3d\r", newSettings->outputFrequency);
		tempValue = uartGetChar();
	}
	printf("\n\n\r");
}

void selectBaudRate(struct settings* newSettings){
	char tempValue=0;
	
	printf("Select the desired baud rate.\n\r");
	printf("[1] 4800\n\r");
	printf("[2] 9600\n\r");
	printf("[3] 14400\n\r");
	printf("[4] 19200\n\r");
	printf("[5] 38400\n\r");
	printf("[6] 57600\n\r");
	printf("[7] 115200\n\r");
	
	tempValue = uartGetChar();
	if(tempValue >= '1' && tempValue <= '7')newSettings->baudRate = tempValue-'1';
	else printf("Invalid Selection!");
	printf("\n\n\r");
}

void setAccelerometerRange(int range){
	if(range == RANGE_60)sbi(PORTD, G_SELECT);
	else if(range == RANGE_15)cbi(PORTD, G_SELECT);
}

void loadSettings(struct settings* newSettings)
{
	newSettings->accelerometerRange = eepromReadInt(EEPROM_SETTINGS_ADDRESS);
	newSettings->outputMode = eepromReadInt(EEPROM_SETTINGS_ADDRESS + 2);
	newSettings->outputFrequency = eepromReadInt(EEPROM_SETTINGS_ADDRESS + 4);
	newSettings->baudRate = eepromReadInt(EEPROM_SETTINGS_ADDRESS + 6);
}

void loadCalibration(struct sensorReadings* calibrationValues)
{
	calibrationValues->x = eepromReadLong(EEPROM_CALIBRATION_ADDRESS);
	calibrationValues->y = eepromReadLong(EEPROM_CALIBRATION_ADDRESS + 4);
	calibrationValues->z = eepromReadLong(EEPROM_CALIBRATION_ADDRESS + 8);
}

void loadSwing(struct sensorReadings* swingValues)
{
	swingValues->x = eepromReadLong(EEPROM_SWING_ADDRESS);
	swingValues->y = eepromReadLong(EEPROM_SWING_ADDRESS + 4);
	swingValues->z = eepromReadLong(EEPROM_SWING_ADDRESS + 8);
}

void saveSettings(struct settings* saveSetting)
{
	eepromWriteInt(EEPROM_SETTINGS_ADDRESS, saveSetting->accelerometerRange);
	eepromWriteInt(EEPROM_SETTINGS_ADDRESS+2, saveSetting->outputMode);
	eepromWriteInt(EEPROM_SETTINGS_ADDRESS+4, saveSetting->outputFrequency);
	eepromWriteInt(EEPROM_SETTINGS_ADDRESS+6, saveSetting->baudRate);
}

void saveCalibration(struct sensorReadings* calibrationValues)
{
	eepromWriteLong(EEPROM_CALIBRATION_ADDRESS, calibrationValues->x);
	eepromWriteLong(EEPROM_CALIBRATION_ADDRESS + 4, calibrationValues->y);
	eepromWriteLong(EEPROM_CALIBRATION_ADDRESS + 8, calibrationValues->z);
}

void saveSwing(struct sensorReadings* swingValues)
{
	eepromWriteLong(EEPROM_SWING_ADDRESS, swingValues->x);
	eepromWriteLong(EEPROM_SWING_ADDRESS + 4, swingValues->y);
	eepromWriteLong(EEPROM_SWING_ADDRESS + 8, swingValues->z);
}