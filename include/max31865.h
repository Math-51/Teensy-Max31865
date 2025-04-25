#ifndef max31865_h
#define max35865_h

#include <Arduino.h>
#include <SPI.h>

/************************** MAX31865 Parameter ***********************************/

#define REGISTER_CONFIG 0x0 // MAX31865 Configuration register adress
#define REGISTER_RTD 0x01   // MAX31865 RTD MSBs register adress
#define REGISTER_FAULT 0x07 // MAX31865 fault register adress
#define RTD_A 3.9083e-3     // Callendar-Van Dusen coefficient values used in equation to convert resistance value into temperature
#define RTD_B -5.775e-7     // Callendar-Van Dusen coefficient values used in equation to convert resistance value into temperature

/*MAX31865 global variables*/

extern int max31865CS;        // CS Pin for MAX31865
extern byte configInit;       // The config byte that will be send to MAX31865 at init. The config bit is determined in max31865Init() function depending on RTD wires number and filter frequency
extern byte configVbias;      // Config byte that will be send to MAX31865 to enable Vbias. The config byte is determined in max31865Init() function, based on configInit byte
extern byte config1Shot;      // Config byte that will be send to MAX31865 to enable 1-shot. The config byte is determined in max31865Init() function, based on configInit & configVbias bytes
extern byte configFaultClear; // Config byte that will clear fault register. The config byte is determined in max31865Init() function, based on configInit byte
extern float refResistor;     // Reference resistor value for calculation of current RTD resistance value
extern float RTDnominal;      // Nominal 0-degrees-C resistance of the RTD for convertion of RTD resistance value into temperature

/*MAX31865 function prototype*/

bool max31865Init(int cs = 23, int wire = 3, int freq = 50, float ref = 4300, float rtd = 1000);
void max31865ReadRTD(word *rtd, bool *fault);
void max31865Calc(float data, float *rt, float *temp, float *R);
void max31865CalcAlt(float data, float *rt, float *temp, float *R);
void max31865ReadFault(byte *faultData);
void max31865ClearFaultRegister();
void max31865Write(byte adress, byte *data);
void max31865Read8(byte adress, byte *data);
void max31865Read16(byte adress, unsigned int *data);

#endif