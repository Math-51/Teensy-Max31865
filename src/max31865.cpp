#include <Arduino.h>
#include "max31865.h"

/*MAX31865 global variables*/

int max31865CS = 0;			 // CS Pin for MAX31865
byte configInit = 0b0;		 // The config byte that will be send to MAX31865 at init. The config bit is determined in max31865Init() function depending on RTD wires number and filter frequency
byte configVbias = 0b0;		 // Config byte that will be send to MAX31865 to enable Vbias. The config byte is determined in max31865Init() function, based on configInit byte
byte config1Shot = 0b0;		 // Config byte that will be send to MAX31865 to enable 1-shot. The config byte is determined in max31865Init() function, based on configInit & configVbias bytes
byte configFaultClear = 0b0; // Config byte that will clear fault register. The config byte is determined in max31865Init() function, based on configInit byte
float refResistor = 0;		 // Reference resistor value for calculation of current RTD resistance value
float RTDnominal = 0;		 // Nominal 0-degrees-C resistance of the RTD for convertion of RTD resistance value into temperature

SPISettings max31865Settting(1000000, MSBFIRST, SPI_MODE1); // MAX31865 SPI settings

/* Function to initialize and configure the MAX31865
- Parameter cs : pas to the function the SPI CS pin used for the MAX31865
- Parameter wire : pass to the function the wires number (2, 3 or 4) of the RTD sensor connected to MAX31865
- Parameter freq : pass to the function the filter frequency that the MAX31865 shall use
- Parameter ref : pass to the function the reference resistor used on the MAX31865
- Parameter rtd : pass to the function the nominal 0-degrees-C resistance of the RTD
Note : this function shall be call one time, to configure the MAX31865
*/
bool max31865Init(int cs, int wire, int freq, float ref, float rtd)
{
	max31865CS = cs;
	refResistor = ref;
	RTDnominal = rtd;


	pinMode(max31865CS, OUTPUT); // Define the CS pin for MAX31865 as an output

	if (wire == 3)
		configInit |= 0b00010000;	 // If RTD sensor connected to MAX31865 is a 3 wires RTD, bit 4 of configuration register shall be flip to 1
	else if (wire != 2 && wire != 4) // If the number passed to the function is different from 3 but also different from 2 & 4, the parameter is wrong
	{
		Serial.println("Wire number error"); // Send a serial message to inform that the wire parameter is wrong
		while (1)
			; // If wire parameter is wrong, stop program execution here
	}

	if (freq == 50)
		configInit |= 0b00000001; // If MAX31865 filter shall be configured to 50Hz, bit 0 of configuration register shall be flip to 1
	else if (freq != 60)		  // If the number passed to the function is different from 50 but also different from 60, the parameter is wrong
	{
		Serial.println("Frequency selection error"); // Send a serial message to inform that the filter frequency parameter is wrong
		while (1)
			; // If filter frequency parameter is wrong, stop program execution here
	}

	// From here, the basic configuration of MAX31865 depending of RTD wire number and MAX31865 filter frequency is defined in the global variable configInit

	configVbias = configInit | 0b10000000;		// Before start a convertion, VBIAS shall be swithed ON via config register bit 7 : define the basic configuration register with VBIAS on
	config1Shot = configInit | 0b10100000;		// To start a convertion, configuration register bit 5 shall be flip to 1 : define the basic configuration register with bit 7 to 1 (VBIAS on) and bit 5 to 1
	configFaultClear = configInit | 0b00000010; // To clear fault register, configuration register bit 1 shall be flip to 1 : define the basic configuration register with bit 1 to 1

	max31865Write(REGISTER_CONFIG, &configInit); // Send to MAX31865 configuration register the basic configuration byte

	byte configInitRead = 0; // Variable to store the retrieve of the MAX31865 configuration register

	max31865Read8(REGISTER_CONFIG, &configInitRead); // Request to MAX31865 the current configuration resgister and store it in the configInitRead variable

	if (configInit == configInitRead)
		return 1; // If the current configuration register is egal to the configuration send previously, the MAX31865 init is successful, max31865Init() function return 1
	else
		return 0; // If the current configuration register is not egal to the configuration send previously, the MAX31865 init is failed, max31865Init() function return 0
}

/* Function to request to MAX31865 a 1-shot RTD measurement, then read the content of the MAX31865 MSB and LSB RTD resistance registers and store the RTD resistance data and fault bit into variables
- Parameter *rtd   : a pointer to the adress of the variable where RTD resistance data shall be stored. The variable shall be a word (16 bits long)
- Parameter *fault : a pointer to the adress of the variable where the fault bit shall be stored. The variable shall be a bool (1 bit long)
Note : RTD resistance data is 15 bits long :
MSB RTD resistance register contain the bit 14 to bit 7 of the RTD resistance data
LSB RTD resistance register bit 7 to bit 1 correspond to the bit 6 to bit 0 of the RTD resistance data
LSB RTD resistance register bit 0 is a fault bit that indicates whether any RTD faults have been detected by MAX31865
See MAX31865 datasheet for more informations
*/
void max31865ReadRTD(word *rtd, bool *fault)
{
	word rtdRegister = 0x0;						  // Variable to store the MSB and LSB data of MAX31865 RTD resistance registers
	max31865Write(REGISTER_CONFIG, &configVbias); // Enable MAX31865 Vbias before request the 1-shot measurement
	delay(10);									  // Wait Vbias charching capacitor
	max31865Write(REGISTER_CONFIG, &config1Shot); // Request to MAX31865 a RTD 1-shot measurement
	delay(65);									  // Wait convertion is done by MAX31865
	max31865Read16(REGISTER_RTD, &rtdRegister);	  // Read RTD register and store retrieve data into rtdRegister variable
	max31865Write(REGISTER_CONFIG, &configInit);  // Shut off Vbias, MAX31865 return into steady-state mode (no convertion), bit 5 (1-shot request) of configuration register is auto-cleared
	*rtd = rtdRegister >> 1;					  // Remove the fault bit and store RTD resistance data into the variable pointed by the pointer
	*fault = rtdRegister & 0x1;					  // Isolate the fault bit and store the fault bit into the variable pointed by the pointer
}

/* Function to calculate from the RTD resistance data : the ratio in float, the resistance value and the temperature, then store them into variables
- Parameter data :  pass to the function the RTD resistance data obtain with the max31865ReadRTD() function
- Parameter *rt :   a pointer to the adress of the variable where the ratio in float shall be store
- Parameter *temp : a pointer to the adress of the variable where the temperature value shall be stored
- Parameter *R :    a pointer to the adress of the variable where the resistance value shall be stored
The calculation method come from the Adafruit_MAX31865::temperature() function of the Adafruit MAX31865 Arduino Library, see: https://github.com/adafruit/Adafruit_MAX31865
The calculation method is detailed in MAX31865 datasheet or here: http://www.analog.com/media/en/technical-documentation/application-notes/AN709_0.pdf
*/
void max31865Calc(float data, float *rt, float *temp, float *R)
{
	float Z1, Z2, Z3, Z4;

	data /= 32768; // Convert RTD resistance data into ratio in float

	*rt = data; // Store the ratio in float into the variable pointed by the pointer

	data *= refResistor; // Convert the ratio into resistance value

	*R = data; // Store resistance value into the variable pointed by the pointer

	Z1 = -RTD_A;
	Z2 = RTD_A * RTD_A - (4 * RTD_B);
	Z3 = (4 * RTD_B) / RTDnominal;
	Z4 = 2 * RTD_B;

	*temp = Z2 + (Z3 * data);
	*temp = (sqrt(*temp) + Z1) / Z4;

	if (*temp >= 0)
		return;

	// ugh.
	data /= RTDnominal;
	data *= 100; // normalize to 100 ohm

	float rpoly = data;

	*temp = -242.02;
	*temp += 2.2228 * rpoly;
	rpoly *= data; // square
	*temp += 2.5859e-3 * rpoly;
	rpoly *= data; // ^3
	*temp -= 4.8260e-6 * rpoly;
	rpoly *= data; // ^4
	*temp -= 2.8183e-8 * rpoly;
	rpoly *= data; // ^5
	*temp += 1.5243e-10 * rpoly;
}

/* Function to read the MAX31865 fault register
- Parameter *faultData : a pointer to the adress of the variable where the fautl value in byte shall be store
*/
void max31865ReadFault(byte *faultData)
{
	byte faultValue = 0x0;						// Variable to retrieve the the fault value
	max31865Read8(REGISTER_FAULT, &faultValue); // Read the fault register and store it
	*faultData = faultValue;					// Store fault data into the variable pointed by the pointer
}

/* Function to request to clear the MAX31865 fault register
*/
void max31865ClearFaultRegister()
{
	max31865Write(REGISTER_CONFIG, &configFaultClear); // Send to MAX31865 configuration register the fault clear configuration byte
}

/* Function to SPI write data transfert to MAX31865
- Parameter adress : pass to the function the MAX31865 adress register where data shall be write
- Parameter *data :  a pointer to the adress of the variable containing the data to transfert to MAX31865 (shall be a byte)
Note : pass to the function the READ adress of the register, it will be automatically converted into a WRITE register adress
*/
void max31865Write(byte adress, byte *data)
{
	adress |= 0x80;							// Write adress register is the same adress than READ, with the MSB flip to 1
	SPI.beginTransaction(max31865Settting); // Initializes the SPI bus with adapted parameters for transmission to MAX31865
	digitalWrite(max31865CS, LOW);			// CS pin of MAX31865 set to LOW
	SPI.transfer(adress);					// Write the register adress on the bus
	SPI.transfer(*data);					// Write the data on the bus
	digitalWrite(max31865CS, HIGH);			// CS pin of MAX31865 set to HIGH
	SPI.endTransaction();					// SPI bus release, to make it available for other transmisions
}

/* Function to SPI read a byte data from MAX31865 register
- Parameter adress : pass to the function the MAX31865 adress register where data shall be read
- Parameter *data :  a pointer to the adress of the variable where the received data shall be stored (shall be a byte)
*/
void max31865Read8(byte adress, byte *data)
{
	SPI.beginTransaction(max31865Settting); // Initializes the SPI bus with adapted parameters for transmission to MAX31865
	digitalWrite(max31865CS, LOW);			// CS pin of MAX31865 set to LOW
	SPI.transfer(adress);					// Write the register adress on the bus
	*data = SPI.transfer(0xFF);				// Dummy write on MOSI (line maintained HIGH) to provide SPI clock signals to read a 8 bits data from MAX31865 on MISO line
	digitalWrite(max31865CS, HIGH);			// CS pin of MAX31865 set to HIGH
	SPI.endTransaction();					// SPI bus release, to make it available for other transmisions
}

/* Function to SPI read 2 concecutives byte data from MAX31865 register
Note : MAX31865 support multiple-byte transfer (see MAX31865 datasheet)
For a multiple-byte transfer, multiple bytes can be read after the address has been written: the address continues to increment through all memory locations as long as CS remains low
- Parameter adress : pass to the function the MAX31865 adress register where multiple-byte read data shall start
- Parameter *data :  a pointer to the adress of the variable where the received data shall be stored (shall be a word)
*/
void max31865Read16(byte adress, word *data)
{
	byte dataMSB = 0x0;						// Variable to store the MSB part of the data
	byte dataLSB = 0x0;						// Variable to store the LSB part of the data
	SPI.beginTransaction(max31865Settting); // Initializes the SPI bus with adapted parameters for transmission to MAX31865
	digitalWrite(max31865CS, LOW);			// CS pin of MAX31865 set to LOW
	SPI.transfer(adress);					// Write the register adress on the bus
	dataMSB = SPI.transfer(0xFF);			// Dummy write on MOSI (line maintained HIGH) to provide SPI clock signals to read a 8 bits data from MAX31865 on MISO line
	dataLSB = SPI.transfer(0xFF);			// Dummy write on MOSI (line maintained HIGH) to provide SPI clock signals to read a 8 bits data from MAX31865 on MISO line
	digitalWrite(max31865CS, HIGH);			// CS pin of MAX31865 set to HIGH
	SPI.endTransaction();					// SPI bus release, to make it available for other transmisions
	*data = dataMSB << 8 | dataLSB;			// Combine MSB and LSB data into a single variable pointed by the pointer *data
}