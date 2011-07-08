/*******************************************************
* Program Specific Structures
*******************************************************/
//Description: Keeps track of the configuration settings for the serial accelerometer.
struct settings{
	int accelerometerRange;	//used to keep track of the accelerometer range (either 1.5G or 6.0G)
	int outputMode;			//keeps track of the desired output mode for accelerometer data (Gravity, Raw or Binary)
	int outputFrequency;	//The frequency at which data will be sent to the serial port. Different limits depending on the selected baud rate.
	unsigned int baudRate;			//The baud rate for serial communications
};

//Description: Stores x, y and z unsigned long integer data. Used for ADC counts and the millivolts and the calibration values
struct sensorReadings{
	unsigned long int x;
	unsigned long int y;
	unsigned long int z;
};

//Descriptions: Used to store x,y and z float data. Used for G Values
struct sensorValues{
	double x;
	double y;
	double z;
};

//=======================================================
//					Function Definitions
//=======================================================
void IOInit(void);
void selectAccelerometerRange(struct settings* newSettings, struct sensorReadings* swingValues);
char configMenu(struct settings* menuSettings, struct sensorReadings* menuCalibrationValues);
void toGValue(struct sensorValues* gValue, struct sensorReadings* voltage, struct sensorReadings* calibration, struct sensorReadings* swing);
void selectCalibrationValues(struct sensorReadings* newCalibrationValues, struct sensorReadings* swingValues);
void selectOutputMode(struct settings* newSettings);
void selectOutputFrequency(struct settings* newSettings);
void selectBaudRate(struct settings* newSettings);
void setAccelerometerRange(int range);
void loadSettings(struct settings* newSettings);
void loadCalibration(struct sensorReadings* calibrationValues);
void saveSettings(struct settings* saveSetting);
void saveCalibration(struct sensorReadings* calibrationValues);
void loadSwing(struct sensorReadings* swingValues);
void saveSwing(struct sensorReadings* swingValues);

/********************************************************
* EEPROM Addresses
********************************************************/
//Note: The first EEPROM address stores the 'first run' information

//Settings will start at EEPROM address 0 and take 8 bytes
// (2 for accelRange, 2 for outputMode, 2 for outputFreq
// ad 2 for baudRate
#define EEPROM_SETTINGS_ADDRESS	1
#define EEPROM_SETTINGS_SIZE	8

//Calibration will start after the Settings in EEPROM and
// will take 12 bytes (4 each for X,Y and Z 0 offset values)
#define EEPROM_CALIBRATION_ADDRESS (EEPROM_SETTINGS_ADDRESS + EEPROM_SETTINGS_SIZE)
#define EEPROM_CALIBRATION_SIZE	12

//Swing values will start after the Calibration values in EEPROM
// and will take 12 bytes
#define EEPROM_SWING_ADDRESS (EEPROM_CALIBRATION_ADDRESS + EEPROM_CALIBRATION_SIZE)
#define EEPROM_SWING_SIZE	12

//*******************************************************
//					GPIO Definitions
//*******************************************************
#define G_SELECT	2
#define Z_AXIS	0
#define Y_AXIS	1
#define X_AXIS	2
#define BOOT_RESET	3

#define LED_PIN	5

//*******************************************************
//						Macros
//*******************************************************
#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

#define ledOn()	sbi(PORTB, LED_PIN);
#define ledOff()	cbi(PORTB, LED_PIN);
#define ledToggle()	sbi(PINB, LED_PIN);

//*******************************************************
//					General Definitions
//*******************************************************
//Define the accelerometer sensitivities in mV/G
//These definitions can also be used to set the accelerometer range
#define RANGE_15	800
#define RANGE_60	206

//Define the number of readings to average
#define NUM_READINGS	4

//Define the output modes for the accelerometer data
#define OUTPUT_GRAVITY	0
#define OUTPUT_RAW	1
#define OUTPUT_BINARY	2

//Define the Baud Rate Selections
#define BAUD_4800	0
#define BAUD_9600	1
#define BAUD_14400	2
#define BAUD_19200	3
#define BAUD_38400	4
#define BAUD_57600	5
#define BAUD_115200	6

//Define the menu selections for the configuration menu
#define MENU_CALIBRATE	'1'
#define MENU_MODE	'2'
#define MENU_FREQUENCY	'3'
#define MENU_RANGE	'4'
#define MENU_BAUD	'5'
#define MENU_EXIT	'X'