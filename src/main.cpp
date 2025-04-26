#include <Arduino.h>
#include "max31865.h"

#define MAX31865_CS_Pin 10     // The CS Pin for MAX31865: to change depending on PIN number of microcontroler used as MAX31865 CS pin
#define WIRE 2                 // The number of wires of the RTD (2, 3 or 4, depending on the wires number of the RTD sensor)
#define FILTER_FREQ 50         // Filter frequency of the MAX31865 (50Hz or 60Hz)
#define REF_RESISTOR 430.0     // The value of the Rref resistor. For Adafruit MAX31865 breakout board, use 430.0 for PT100 board and 4300.0 for PT1000 board
#define R_NOMINAL 100.0        // The 'nominal' 0-degrees-C resistance of the sensor: 100.0 for PT100, 1000.0 for PT1000
#define SAMPLE_INTERVAL 500    // The interval between 2 temperature measurement

word rtdData = 0;             // Variable to store last readed MAX31865 RTD register RTD resistance data (RTD MSB resister and RTD LSB register bit 7 to 1)
bool faultBit = 0;            // Variable to store last readed RTD register fault bit (bit 0 of RTD LSB register)
float ratio = 0;              // Variable to store the last readed MAX31865 ratio converted into float
float resistance = 0;         // Variable to store the last RTD resistance value calculated from the ratio
float temperature = 0;        // Variable to store the last temperature value calculated from resistance value
byte faultRegister = 0;       // Variable to store MAX31865 fault resister if 10 concecutives faults are detected

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

}

void loop() {

    thermo.readRTD(&rtdData, &faultBit); // Use the readRTD method of the thermo object

    thermo.calculate(rtdData, &ratio, &temperature, &resistance); // Use the calculate method of the thermo object

    thermo.readFault(&faultRegister); // Use the readFault method of the thermo object

    Serial.print("Temperature = ");
    Serial.println(temperature); // Send temperature value in float to serial

    thermo.calculateAlt(rtdData, &ratio, &temperature, &resistance); // Use the calculateAlt method of the thermo object
    Serial.print("Temperature (alt) = ");
    Serial.println(temperature); // Send temperature value in float to serial

    Serial.print("Fault = 0x");
    Serial.println(faultRegister, HEX); // Send fault register value in HEX to serial

    thermo.clearFaultRegister(); // Use the clearFaultRegister method of the thermo object

    delay(SAMPLE_INTERVAL);

}