#include <Arduino.h>
#include "max31865.h"

#define CONTINUS
#define SAMPLE_INTERVAL_CONTINUOUS 20 // The interval between 2 temperature measurement in continuous mode
#define SAMPLE_INTERVAL_1SHOT 500    // The interval between 2 temperature measurement
#define MAX31865_CS_Pin 10     // The CS Pin for MAX31865: to change depending on PIN number of microcontroler used as MAX31865 CS pin
#define WIRE 2                 // The number of wires of the RTD (2, 3 or 4, depending on the wires number of the RTD sensor)
#define FILTER_FREQ 50         // Filter frequency of the MAX31865 (50Hz or 60Hz)
#define REF_RESISTOR 430.0     // The value of the Rref resistor. For Adafruit MAX31865 breakout board, use 430.0 for PT100 board and 4300.0 for PT1000 board
#define R_NOMINAL 100.0        // The 'nominal' 0-degrees-C resistance of the sensor: 100.0 for PT100, 1000.0 for PT1000

Max31865 thermo(MAX31865_CS_Pin,REF_RESISTOR, R_NOMINAL); // Create an object of Max31865 class

void setup() {
    Serial.begin(115200);
    
    while (!Serial) {
        // wait for Arduino Serial Monitor to be ready
    }

    if (!thermo.config(WIRE, FILTER_FREQ)) { // Use the init method of the thermo object
        Serial.println("MAX31865 Init fail");
        while (1) {
            delay(1000); // If init failed, stop program execution here
        }
    }
    Serial.println("MAX31865 init OK"); // If MAX31865 init succeed , send a serial message to confirm the init of MAX31865

    #ifdef CONTINUS
    if (!thermo.enableContinuousMode()) { // Use the enableContinuousMode method of the thermo object
        Serial.println("MAX31865 Continuous mode fail");
        while (1) {
            delay(1000); // If continuous mode failed, stop program execution here
        }
    }
    Serial.println("MAX31865 Continuous mode OK"); // If continuous mode succeed, send a serial message to confirm the continuous mode of MAX31865
    #endif

}

void loop() {
    static uint16_t rtdData = 0;         // Variable to store last read MAX31865 RTD register RTD resistance data
    static bool faultBit = 0;            // Variable to store last readed RTD register fault bit (bit 0 of RTD LSB register)
    static float ratio = 0;              // Variable to store the last readed MAX31865 ratio converted into float
    static float resistance = 0;         // Variable to store the last RTD resistance value calculated from the ratio
    static float temperature = 0;        // Variable to store the last temperature value calculated from resistance value
    static uint8_t faultRegister = 0;    // Variable to store MAX31865 fault register if 10 consecutive faults are detected

    #ifdef CONTINUS
    thermo.continusReadRTD(&rtdData, &faultBit); // Use the continusReadRTD method of the thermo object
    #endif

    #ifndef CONTINUS
    thermo.oneShotReadRTD(&rtdData, &faultBit); // Use the readRTD method of the thermo object
    #endif

    thermo.calculateTemperatureAdAlgo(rtdData, &ratio, &temperature, &resistance); // Use the calculate method of the thermo object

    if (faultBit) { 
        thermo.readFault(&faultRegister); // Use the readFault method of the thermo object
        thermo.clearFaultRegister(); // Use the clearFaultRegister method of the thermo object
    }

    Serial.print("Temperature = ");
    Serial.println(temperature); // Send temperature value in float to serial

    thermo.calculateTemperaturePT100Algo(rtdData, &ratio, &temperature, &resistance); // Use the calculateAlt method of the thermo object
    Serial.print("Temperature (alt) = ");
    Serial.println(temperature); // Send temperature value in float to serial

    Serial.print("Fault = 0x");
    Serial.println(faultRegister, HEX); // Send fault register value in HEX to serial

    #ifdef CONTINUS
    delay(SAMPLE_INTERVAL_CONTINUOUS); // Wait for 20ms
    #endif

    #ifndef CONTINUS
    delay(SAMPLE_INTERVAL_1SHOT);
    #endif
}