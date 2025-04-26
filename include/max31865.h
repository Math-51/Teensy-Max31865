#ifndef max31865_h
#define max31865_h

#include <SPI.h>

/************************** MAX31865 Parameter ***********************************/

#define REGISTER_CONFIG 0x0 // MAX31865 Configuration register address
#define REGISTER_RTD 0x01   // MAX31865 RTD MSBs register address
#define REGISTER_FAULT 0x07 // MAX31865 fault register address
#define RTD_A 3.9083e-3     // Callendar-Van Dusen coefficient values used in equation to convert resistance value into temperature
#define RTD_B -5.775e-7     // Callendar-Van Dusen coefficient values used in equation to convert resistance value into temperature

class Max31865 {
private:
    int m_cs;                // CS Pin for MAX31865
    byte m_configInit;       // The config byte that will be sent to MAX31865 at init
    byte m_configVbias;      // Config byte to enable Vbias
    byte m_config1Shot;      // Config byte to enable 1-shot
    byte m_configFaultClear; // Config byte to clear fault register
    float m_refResistor;     // Reference resistor value
    float m_rtdNominal;      // Nominal 0-degrees-C resistance of the RTD

    SPISettings m_max31865Setting;

    void write(byte address, byte *data);
    void read8(byte address, byte *data);
    void read16(byte address, unsigned int *data);

public:
    Max31865(int cs);
    bool init(int wire = 3, int freq = 50, float ref = 4300, float rtd = 1000);
    void readRTD(word *rtd, bool *fault);
    void calculate(float data, float *rt, float *temp, float *R);
    void calculateAlt(float data, float *rt, float *temp, float *R);
    void readFault(byte *faultData);
    void clearFaultRegister();
};

#endif