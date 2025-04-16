#include <Arduino.h>
#include <SPI.h>
#include "max31865.h"

#define MAX31865_CS_Pin 10    // The CS Pin for MAX31865: to change depending on PIN number of microcontroler used as MAX31865 CS pin
#define WIRE 3                // The number of wires of the RTD (2, 3 or 4, depending on the wires number of the RTD sensor)
#define FILTER_FREQ 50        // Filter frequency of the MAX31865 (50Hz or 60Hz)
#define REF_RESISTOR 4300.0   // The value of the Rref resistor. For Adafruit MAX31865 breakout board, use 430.0 for PT100 board and 4300.0 for PT1000 board
#define R_NOMINAL 1000.0      // The 'nominal' 0-degrees-C resistance of the sensor: 100.0 for PT100, 1000.0 for PT1000
#define SAMPLE_INTERVAL 500   // The interval between 2 temperature measurement

word rtdData = 0;             // Variable to store last readed MAX31865 RTD register RTD resistance data (RTD MSB resister and RTD LSB register bit 7 to 1)
bool faultBit = 0;            // Variable to store last readed RTD register fault bit (bit 0 of RTD LSB register)
float ratio = 0;              // Variable to store the last readed MAX31865 ratio converted into float
float resistance = 0;         // Variable to store the last RTD resistance value calculated from the ratio
float temperature = 0;        // Variable to store the last temperature value calculated from resistance value
byte faultRegister = 0;       // Variable to store MAX31865 fault resister if 10 concecutives faults are detected

void setup() {

  Serial.begin(115200);

  while (!Serial) {
    // wait for Arduino Serial Monitor to be ready
  }

  Serial.println("Test point 1"); // Test point

  if (max31865Init(MAX31865_CS_Pin, WIRE, FILTER_FREQ, REF_RESISTOR, R_NOMINAL) == 0) // Init MAX31865 and configure the RTD wires number and MAX31865 filter frequency
  {
    while (1)
    Serial.println("MAX31865 Init fail"); // If max31865Init() function return 0, the init of MAX31865 has failed: send a serial message to inform that init as failed
    delay(1000); // If init failed, stop program execution here
  }
  Serial.println("MAX31865 init OK"); // If MAX31865 init succeed , send a serial message to confirm the init of MAX31865

}

void loop() {

  max31865ReadRTD(&rtdData, &faultBit);   // Read MAX31865 RTD resistance register

  max31865Calc(rtdData, &ratio, &temperature, &resistance); // From the 15bits RTD resistance data of MAX31865 RTD resistance registers, convert the ratio in float, calculate the resistance value and the temperature value

  Serial.print("Temperature = ");
  Serial.println(temperature); // Send temperature value in float to serial
  Serial.print("Fault = ");
  Serial.println(faultRegister, HEX); // Send fault register value in HEX to serial

  delay(SAMPLE_INTERVAL);

}