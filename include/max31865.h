#ifndef max31865_h
#define max31865_h

#include <SPI.h>

/************************** MAX31865 Parameter ***********************************/

#define REGISTER_CONFIG 0x0 // MAX31865 Configuration register address
#define REGISTER_RTD 0x01   // MAX31865 RTD MSBs register address
#define REGISTER_FAULT 0x07 // MAX31865 fault register address
#define RTD_A 3.9083e-3     // Callendar-Van Dusen coefficient values used in equation to convert resistance value into temperature
#define RTD_B -5.775e-7     // Callendar-Van Dusen coefficient values used in equation to convert resistance value into temperature
#define KA 4.261676e-7
#define KB 1.1252e-5
#define KC 0.00105387
#define KD 2.558959

class Max31865 {
private:
    uint8_t m_cs;                // CS Pin for MAX31865
    uint8_t m_configInit;       // The config byte that will be sent to MAX31865 at init
    uint8_t m_configVbias;      // Config byte to enable Vbias
    uint8_t m_config1Shot;      // Config byte to enable 1-shot
    uint8_t m_configFaultClear; // Config byte to clear fault register
    float m_refResistor;     // Reference resistor value
    float m_rtdNominal;      // Nominal 0-degrees-C resistance of the RTD

    SPISettings m_max31865Setting;

    void write(uint8_t address, uint8_t *data);
    void read8(uint8_t address, uint8_t *data);
    void read16(uint8_t address, uint16_t *data);

public:
    Max31865(uint8_t cs, float refResistor, float rtdNominal);
    bool config(uint8_t wire = 3, uint8_t filterFreq = 50);
    bool enableContinuousMode();
    bool disableContinuousMode();
    void oneShotReadRTD(uint16_t *rtd, bool *fault);
    void continusReadRTD(uint16_t *rtd, bool *fault);
    void calculateTemperatureAdAlgo(float data, float *rt, float *temp, float *R);
    void calculateTemperaturePT100Algo(float data, float *rt, float *temp, float *R);
    void readFault(uint8_t *faultData);
    void clearFaultRegister();
};

#endif